#include <limits.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ble.h"
#include "esp_log.h"
#include "lvgl.h"
#include "display.h"
#include "ui.h"

static void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = lv_area_get_width(area);
    const uint32_t h = lv_area_get_height(area);

    // TODO: Shift out the pixels to the it8951
    
    // This function must be called when the display has been updated
    lv_disp_flush_ready(disp);
}

static inline uint32_t custom_tick_get(void) {
    // The function returns a int64_t in [us], so we cast to [ms] 
    return (uint32_t)(esp_timer_get_time()/1000);
}

// TODO: Try to speed up compilation by removing unnecessary components
// TODO: The serial output returns "ESP-IDF: 5.2.1", while the latest ESP-IDF is
// 5.3 for the S3... Why is this the case? PIO seems updated
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
        lv_task_handler();
        lv_timer_handler();
    }

    lv_deinit();
}
