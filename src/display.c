#include "it8951.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "hal/spi_ll.h"
#include "lvgl.h"
#include "display.h"

static stIT8951_Handler_t it8951_hdlr;
static spi_device_handle_t spi;

static const gpio_num_t ncs = 34;
static const gpio_num_t hrdy = 9;
static const gpio_num_t spi_mosi = 35;
static const gpio_num_t spi_miso = 37;
static const gpio_num_t spi_clk = 36;

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

// Max DMA transfer byte length on the ESP32-S3 is (1<<18)/8=32768. In a single 
// transaction, the maximum amount of bytes we may want to tx is a full image at
// 4bpp: 1872*1404/2. Therefore an SPI queue size of 
// ceil((1872*1404/2)/32768)=41 can utilize the SPI+DMA fully.
#define SPI_MAX_TRANSFER_SIZE (SPI_LL_DMA_MAX_BIT_LEN/8)
// TODO: This needs a generic ROUND_UP macro
#define SPI_QUEUE_SIZE (((DISPLAY_HOR_RES*DISPLAY_VER_RES)/(SPI_MAX_TRANSFER_SIZE*2ull))+1)
static_assert(SPI_MAX_TRANSFER_SIZE*8 == SPI_LL_DMA_MAX_BIT_LEN, "SPI_LL_DMA_MAX_BIT_LEN must be a whole number of bytes");
static_assert(DISPLAY_UI_H % 4 == 0, "UI height must be a multiple of 4 for the IT8951 x alignment");

static inline bool it8951_transcieve(const void *txdata, void *rxdata, size_t len) {
    // TODO: Could keep the CS active between transmissions! Would allow lower 
    // power consumption .flags = SPI_TRANS_CS_KEEP_ACTIVE
    // TODO: Is there a speed/power gain by using the .tx_data, .rx_data and 
    // .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA for len <= 4?
    // TODO: Implement queues to speed things up!

    int32_t bit_size = len*8;
    uint32_t ptr_off = 0;
    bool status = true;
    while(bit_size > 0 && status) {
        const int32_t bit_to_tx = min(SPI_LL_DMA_MAX_BIT_LEN, bit_size);
        status = spi_device_polling_transmit(spi, &(spi_transaction_t) {
            .length = bit_to_tx,
            .tx_buffer = txdata ? (txdata + ptr_off) : NULL,
            .rx_buffer = rxdata ? (rxdata + ptr_off) : NULL,
        }) == ESP_OK;
        bit_size -= bit_to_tx;
        ptr_off += bit_to_tx/8;
    }
    return status;
}

FORCE_INLINE_ATTR void it8951_set_ncs(bool state) {
    gpio_set_level(ncs, state);
}

static SemaphoreHandle_t hrdy_semaphore;
void IRAM_ATTR hrdy_isr(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)arg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

FORCE_INLINE_ATTR void it8951_wait_hrdy(void) {
    // The semaphore may hold a stale token from an earlier edge, so re-check the level
    while(gpio_get_level(hrdy) == 0) {
        // TODO: Using xTaskToNotify may be a quicker option
        xSemaphoreTake(hrdy_semaphore, portMAX_DELAY);
    }
}

// Packed 4bpp copy of the rotated partial buffer that is sent to the IT8951
EXT_RAM_BSS_ATTR static uint8_t gray4_buff[DISPLAY_DRAW_BUF_SIZE/2];

// Rotates the L8 partial render (w x h) into a packed 4bpp image of h x w
// pixels (UI x -> panel -y, UI y -> panel x). The source is read in tiles of 
// 16 rows so that the cache lines of the PSRAM source stay hot.
__attribute__((optimize("Ofast")))
static void l8_to_rotated_gray4(const uint8_t *src, int32_t w, int32_t h, uint32_t stride, uint8_t *dst) {
    const int32_t dst_stride = h >> 1;
    for(int32_t yb = 0; yb < h; yb += 16) {
        const int32_t ye = min(yb + 16, h);
        for(int32_t py = 0; py < w; py++) {
            const uint8_t *p = src + yb*stride + (w - 1 - py);
            uint8_t *o = dst + py*dst_stride + (yb >> 1);
            for(int32_t y = yb; y < ye; y += 2, p += 2*stride) {
                *o++ = (p[0] & 0xF0) | (p[stride] >> 4);
            }
        }
    }
}

void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    static const stIT8951_ImageInfo_t img_info = {
        .rotation = IT8951_ROTATION_MODE_0,
        .bpp = IT8951_COLOR_DEPTH_BPP_4BIT,
        .endianness = IT8951_ENDIANNESS_BIG,
    };
    // Bounding box of everything written since the last refresh
    static int32_t x1, y1, x2, y2;
    static bool dirty = false;

    const int32_t w = lv_area_get_width(area);
    const int32_t h = lv_area_get_height(area);
    // display_rounder() guarantees that h is a multiple of 4
    const stRectangle_t rect = {
        .x      = area->y1,
        .y      = DISPLAY_UI_W - 1 - area->x2,
        .width  = h,
        .height = w
    };

    l8_to_rotated_gray4(px_map, w, h, lv_draw_buf_width_to_stride(w, LV_COLOR_FORMAT_L8), gray4_buff);
    it8951_write_packed_pixels(&it8951_hdlr, &img_info, &rect, gray4_buff, (uint32_t)w*h);

    if(!dirty) {
        x1 = rect.x; y1 = rect.y;
        x2 = rect.x + rect.width; y2 = rect.y + rect.height;
        dirty = true;
    } else {
        x1 = min(x1, rect.x); y1 = min(y1, rect.y);
        x2 = max(x2, rect.x + rect.width); y2 = max(y2, rect.y + rect.height);
    }

    // Refresh the panel once, after the last chunk of this render cycle
    if(lv_display_flush_is_last(disp)) {
        const stRectangle_t refresh = { .x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1 };
        it8951_display_area(&it8951_hdlr, &refresh, IT8951_DISPLAY_MODE_GC16);
        dirty = false;
    }

    lv_display_flush_ready(disp);
}

__attribute__((optimize("Ofast"))) 
void IRAM_ATTR display_rounder(lv_event_t *e) {
    // The IT8951 expects the x coordinates of the rectangle to be padded to the 
    // nearest (16/bpp) pixel (padding to 4 pixels in our case). The UI is 
    // rotated by 90 degrees before it is sent, so the panel's x axis is the 
    // UI's y axis. Expanding the invalidated area so that y1 and y2+1 always 
    // land on a 4 pixel boundary meets this with no post-processing. The panel
    // x offset is either y1 or DISPLAY_UI_H-1-y2, both multiples of 4.
    lv_area_t * area = lv_event_get_param(e);
    area->y1 =   area->y1    & ~0b11;
    // y2 is an inclusive pixel bound!
    area->y2 = ((area->y2+4) & ~0b11)-1;
}

void display_init(void) {
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &(spi_bus_config_t){
        .miso_io_num = spi_miso,
        .mosi_io_num = spi_mosi,
        .sclk_io_num = spi_clk,
        .quadwp_io_num = -1, // Unused
        .quadhd_io_num = -1, // Unused
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
    }, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &(spi_device_interface_config_t){
        .clock_speed_hz = 24000000,
        .mode = 0,
        .spics_io_num = -1, // Controlled externally
        .queue_size = SPI_QUEUE_SIZE,
        .pre_cb = NULL,
        .post_cb = NULL,
        .flags = 0,
    }, &spi));

    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << hrdy),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    });
    hrdy_semaphore = xSemaphoreCreateBinary();
    gpio_install_isr_service(0);
    gpio_isr_handler_add(hrdy, hrdy_isr, (void*)hrdy_semaphore);

    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << ncs),
        .mode = GPIO_MODE_DEF_OUTPUT,
    });
    gpio_set_level(ncs, true);
    
    it8951_hdlr = (stIT8951_Handler_t) {
        .spi_transcieve = it8951_transcieve,
        .set_ncs        = it8951_set_ncs,
        .wait_hrdy      = it8951_wait_hrdy,
        .vcom_mv        = INT_MAX,
    };
    it8951_init(&it8951_hdlr);

    // Clear the display to white
    it8951_fill_rect(&it8951_hdlr, &it8951_hdlr.panel_area, IT8951_DISPLAY_MODE_INIT, 0xF);
}