#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "ble.h"
#include "it8951.h"
#include "esp_log.h"

static spi_device_handle_t spi = {
};

static gpio_num_t ncs = 34;
static gpio_num_t hrdy = 9;

static inline bool it8951_transcieve(const void *txdata, void *rxdata, size_t len) {
    return spi_device_transmit(spi, &(spi_transaction_t) {
        .length = len*8,
        .tx_buffer = txdata,
        .rx_buffer = rxdata
    }) == ESP_OK;
}

static inline void it8951_set_ncs(bool state) {
    gpio_set_level(ncs, state);
}

static inline bool it8951_get_hrdy(void) {
    return gpio_get_level(hrdy);
}

// TODO: Try to speed up compilation by removing unnecessary components
// TODO: The serial output returns "ESP-IDF: 5.2.1", while the latest ESP-IDF is
// 5.3 for the S3... Why is this the case? PIO seems updated
void app_main(void) {
    ESP_LOGI("HA-EINK", "Starting HA E-Ink display...");
    
    ble_init();

    //stIT8951_Handler_t hdlr = {
    //    .spi_transcieve = it8951_transcieve,
    //    .set_ncs        = it8951_set_ncs,
    //    .get_hrdy       = it8951_get_hrdy,
    //    .vcom_mv        = INT_MAX,
    //};
    //it8951_init(&hdlr);

    //lv_init();
    //lv_obj_t *label = lv_label_create(lv_scr_act());
    //lv_label_set_text(label, "Hello World!");
    ESP_LOGI("CHIP_INFO", "PSRAM: %dMB", heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / (1024 * 1024));

    // Enter a 1s background loop (this is not required if the setup is complete)
    while(true) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
