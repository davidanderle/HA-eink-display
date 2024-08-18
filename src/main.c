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
#include "esp_spiffs.h"

// TODO: Put these in a project.h or something...
const char *const json_path = "/spiffs/ui_data.json";
volatile bool is_json_modified = false;
// Mutex for protecting file access
SemaphoreHandle_t file_mutex;

// TODO: Using TinyUSB, an USB mass storage device should be implemented to
// store json files that are displayed on the UI. This setup is rather involved.
// TODO: Separate the IT8951 in a git submodule and add it as a library
// TODO: Try to speed up compilation by removing unnecessary components
// TODO: ESP32-S3/esptool has a bug that sometimes prevents the unit from
// rebooting from the bootloader using the RTS/DTS pins (see here: 
// https://github.com/espressif/arduino-esp32/issues/6762#issuecomment-1182821492)
// Try to set ESP_CONSOLE_USB_SERIAL_JTAG in the menuconfig (prevents USB mass 
// storage from working) and set init state of RTS/DTS to 0 at upload
// TODO: I should reduce the SPIFFS size to allow more flash space for UI elems
// TODO: Although the label's dynamic text update over BLE works, it leaves a 
// weird artefact on the display

// TODO: Could merge the ext_montserrat_14 and the lv_montserrat_14 (to get the
// extra icon glyps) to save some flash
void app_main(void) {
    // 1872x1404 E-Ink VB3300-KCA 4bpp display
    // Buffer for LVGL drawing and rendering (ping-pong). Each buffer is a 1/2
    // frame at 16bit resolution. Before sending the image to the contoller, the 
    // resolution is reduced to 4bpp grayscale. ~5.01MB buffer in SPIRAM
    EXT_RAM_BSS_ATTR static uint8_t draw_buff[2][DISPLAY_VER_RES*DISPLAY_HOR_RES];

    ESP_LOGI("HA-EINK", "Starting HA E-Ink display...");
    
    // Initialize SPIFFS (Serial Peripheral Interface Flash File System). Used
    // to store the JSON file that the UI loads. The file is updated thru BLE
    esp_vfs_spiffs_register(&(esp_vfs_spiffs_conf_t) {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 1,
      .format_if_mount_failed = true
    });
    file_mutex = xSemaphoreCreateMutex();
    if(file_mutex == NULL){
        ESP_LOGE("main", "Failed to create mutex");
    }

    ble_init();

    display_init();

    // Set up LVGL
    lv_init();
    lv_tick_set_cb(xTaskGetTickCount);
    // Create the display and attach the displaying function
    lv_display_t *disp = lv_display_create(DISPLAY_HOR_RES, DISPLAY_VER_RES);
    lv_display_set_antialiasing(disp, true);
    lv_display_set_flush_cb(disp, display_flush);
    lv_display_add_event_cb(disp, display_rounder, LV_EVENT_INVALIDATE_AREA, NULL);
    lv_display_set_buffers(disp, draw_buff[0], draw_buff[1], sizeof(draw_buff), LV_DISPLAY_RENDER_MODE_PARTIAL);
    // Create the UI
    ui_init();

    // Enter a 10ms background loop (this is not required if the setup is complete)
    while(true) {
        // TODO: Sleep management https://docs.lvgl.io/master/porting/sleep.html
        vTaskDelay(pdMS_TO_TICKS(10));
        // All UI operations must be called from the thread this funciton is 
        // periodically called from.
        lv_timer_handler();

        ui_update(NULL);
    }

    lv_deinit();
}
