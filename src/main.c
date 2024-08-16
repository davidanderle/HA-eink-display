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
// TODO: ESP32-S3/esptool has a bug that sometimes prevents the unit from
// rebooting from the bootloader using the RTS/DTS pins (see here: 
// https://github.com/espressif/arduino-esp32/issues/6762#issuecomment-1182821492)
// Try to set ESP_CONSOLE_USB_SERIAL_JTAG in the menuconfig (prevents USB mass 
// storage from working) and set init state of RTS/DTS to 0 at upload

static inline uint32_t custom_tick_get(void) {
    // The function returns a int64_t in [us], so we cast to [ms] 
    return (uint32_t)(esp_timer_get_time()/1000);
}

// TODO: Turn off unused LVGL widgets!
void app_main(void) {
    // 1872x1404 E-Ink VB3300-KCA 4bpp display
    // Buffer for LVGL drawing and rendering (ping-pong). Each buffer is a 1/2
    // frame at 16bit resolution. Before sending the image to the contoller, the 
    // resolution is reduced to 4bpp grayscale. ~5.01MB buffer in SPIRAM
    EXT_RAM_BSS_ATTR static uint8_t draw_buff[2][DISPLAY_VER_RES*DISPLAY_HOR_RES];

    ESP_LOGI("HA-EINK", "Starting HA E-Ink display...");
    
    ble_init();

    display_init();

    lv_init();

    lv_tick_set_cb(custom_tick_get);

    // Create the display and attach the displaying function
    lv_display_t *disp = lv_display_create(DISPLAY_HOR_RES, DISPLAY_VER_RES);
    lv_display_set_antialiasing(disp, true);
    lv_display_set_flush_cb(disp, display_flush);
    lv_display_set_buffers(disp, draw_buff[0], draw_buff[1], sizeof(draw_buff), LV_DISPLAY_RENDER_MODE_PARTIAL);

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
