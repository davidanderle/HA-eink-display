#include <limits.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "ble.h"
#include "esp_log.h"
#include "lvgl.h"
#include "display.h"
#include "ui.h"
#include "it8951.h"

// TODO: Using TinyUSB, an USB mass storage device should be implemented to
// store json files that are displayed on the UI. This setup is rather involved.
// TODO: Separate the IT8951 in a git submodule and add it as a library
// TODO: Try to speed up compilation by removing unnecessary components

static inline uint32_t custom_tick_get(void) {
    // The function returns a int64_t in [us], so we cast to [ms] 
    return (uint32_t)(esp_timer_get_time()/1000);
}

void app_main(void) {
    //1872x1404
    // No need to worry about double-buffering as here FPS is not important...
    static lv_color_t draw_buff[1024];

    ESP_LOGI("CHIP_INFO", "PSRAM: %dMB", heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / (1024 * 1024));
    ESP_LOGI("HA-EINK", "Starting HA E-Ink display...");
    
    ble_init();

    display_init();

    lv_init();

    lv_tick_set_cb(custom_tick_get);

    // Create the display and attach the displaying function
    lv_display_t *disp = lv_display_create(1404, 1872);
    lv_display_set_flush_cb(disp, display_flush);
    // No need to worry about double-buffering as here the FPS is not important
    lv_display_set_buffers(disp, draw_buff, NULL, sizeof(draw_buff), LV_DISPLAY_RENDER_MODE_PARTIAL);

    ui_init();

    // TODO: May want to create a task for the GUI
    //xTaskCreatePinnedToCore(guiTask, "gui", 8192, NULL, 0, NULL, 1);

    // Enter a 1ms background loop (this is not required if the setup is complete)
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        lv_timer_handler();
    }

    lv_deinit();
}
