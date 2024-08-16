#include "it8951.h"
#include "esp_log.h"
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

// Max DMA transfer byte length on the ESP32-S3 is (1<<18)/8=32768. In a single 
// transaction, the maximum amount of bytes we may want to tx is a full image at
// 4bpp: 1872*1404/2. Therefore an SPI queue size of 
// ceil((1872*1404/2)/32768)=41 can utilize the SPI+DMA fully.
#define SPI_MAX_TRANSFER_SIZE (SPI_LL_DMA_MAX_BIT_LEN/8)
// TODO: This needs a generic ROUND_UP macro
#define SPI_QUEUE_SIZE (((DISPLAY_HOR_RES*DISPLAY_VER_RES)/(SPI_MAX_TRANSFER_SIZE*2ull))+1)
static_assert(SPI_MAX_TRANSFER_SIZE <= SPI_MAX_TRANSFER_SIZE, "Adjust SPI transfer size");

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

static inline void it8951_set_ncs(bool state) {
    gpio_set_level(ncs, state);
}

static inline bool it8951_get_hrdy(void) {
    return gpio_get_level(hrdy);
}

__attribute__((pure, optimize("Ofast"))) 
static inline uint8_t IRAM_ATTR rgb565_to_gray4(const uint16_t color) {
    static const uint8_t DRAM_ATTR lut[65536] = { 
        #include "rgb565_to_gray4.txt" 
    };
    return lut[color];
}
// TODO: May need to remove the strict timing dependency of the dirty pixel
// checking by following this: 
//https://docs.lvgl.io/master/porting/display.html#decoupling-the-display-refresh-timer
// This way the display can be forced to refresh after a wake-up event
__attribute__((optimize("Ofast")))
void IRAM_ATTR display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    static const stIT8951_ImageInfo_t img_info = {
        .rotation = IT8951_ROTATION_MODE_0,
        .bpp = IT8951_COLOR_DEPTH_BPP_4BIT,
        .endianness = IT8951_ENDIANNESS_BIG,
    };
    const stRectangle_t disp_rect = {
        .x      = area->x1,
        .y      = area->y1,
        .width  = lv_area_get_width(area),
        .height = lv_area_get_height(area)
    };
    
    // The IT8951 expects the x coordinates of the rectangle to be padded to the 
    // nearest (16/bpp) pixel (padding to 4 pixels in our case)
    // So a 2x1 rectangle at (3,0) coordinate will be sent as a 8x1 rectangle,
    // like d0,d1,d2,p0,p1,d3,d4,d5, where 'dN' are dummy pixels and 'pN' are 
    // the actual pixels. At 4bpp (4pixels per 16bit word) this will give
    // 2 transmit words: 0xYXXX, 0xXXXZ, where Y=p0 and Z=p1, X=don't care
    const uint32_t x1 = area->x1 & ~0b11;
    const uint32_t x2 = (area->x2+3) & ~0b11;
    const stRectangle_t pad_rect = {
        .x      = x1,
        .y      = disp_rect.y,
        .width  = (x2-x1),
        .height = disp_rect.height
    };
    
    //char buff[64];
    //ESP_LOGI("Flush", "Rect=\n%s", rectangle_to_string(&disp_rect, buff));
    //ESP_LOGI("Flush", "Pad rect=\n%s", rectangle_to_string(&pad_rect, buff));

    // Get the number of pixels need to display
    const uint32_t num_pix = lv_area_get_size(area);
    // Convert the RGB565 pixel to GRAY4 and pack them back to the OG array.
    // *px_map is contained within the draw_buff we defined in main.c. When the 
    // flush_cb is called, LVGL is done with the processing of this buffer,
    // therefore we're free to further modify it. Here we map 2 bytes into 4bits
    uint16_t *color = (uint16_t*)px_map;
    for(uint32_t i=0; i<num_pix; i++) {
        // This loop should naturally stay within bounds
        const uint8_t g4 = rgb565_to_gray4(*color++);
        const bool odd = i & 0b1;
        const uint32_t idx = i >> 1;
        px_map[idx] = odd ? (px_map[idx] | (g4 << 4)) : g4;
    }

    // Offset back the pix pointer by the padding (data is don't care)
    uint8_t *ptr = px_map - (disp_rect.x-pad_rect.x)/2;
    const uint32_t num_pix_tx = rectangle_get_area(&pad_rect);
    it8951_write_packed_pixels(&it8951_hdlr, &img_info, &pad_rect, ptr, num_pix_tx);
    it8951_display_area(&it8951_hdlr, &disp_rect, IT8951_DISPLAY_MODE_GC16);

    // lv_display_flush_is_last(display) to check if the last block of rendering

    // This function must be called when the display has been updated
    lv_disp_flush_ready(disp);
}

// TODO: Use observers to bind data to UI elements
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
    });
    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << ncs),
        .mode = GPIO_MODE_DEF_OUTPUT,
    });
    gpio_set_level(ncs, true);
    
    it8951_hdlr = (stIT8951_Handler_t) {
        .spi_transcieve = it8951_transcieve,
        .set_ncs        = it8951_set_ncs,
        .get_hrdy       = it8951_get_hrdy,
        .vcom_mv        = INT_MAX,
    };
    it8951_init(&it8951_hdlr);

    // Clear the display to white
    it8951_fill_rect(&it8951_hdlr, &it8951_hdlr.panel_area, IT8951_DISPLAY_MODE_INIT, 0xF);

    
    //stIT8951_ImageInfo_t img_info = {IT8951_ENDIANNESS_LITTLE, IT8951_COLOR_DEPTH_BPP_4BIT, IT8951_ROTATION_MODE_0};
    // Create a rainbow
    //stRectangle_t rect = {0, 0, it8951_hdlr.panel_area.width/16, it8951_hdlr.panel_area.height};
    //for(uint16_t i=0; i<16; i++){
    //    rect.x = i*rect.width;
    //    it8951_fill_rect(&it8951_hdlr, &rect, IT8951_DISPLAY_MODE_GC16, i);
    //}
}