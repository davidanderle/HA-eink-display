#include <limits.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "ble.h"
#include "battery.h"
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

static uint32_t lv_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

#define UI_TASK_STACK_SIZE (16*1024)
#define BATTERY_PERIOD_MS  (60*1000)

// LVGL's rendering needs far more stack than the default main task provides, so
// everything LVGL related runs in a dedicated task.
static void ui_task(void *arg) {
    ESP_UNUSED(arg);
    // 1872x1404 E-Ink VB3300-KCA 4bpp display, mounted in portrait. LVGL 
    // renders in L8 into two ping-pong buffers, each a 1/2 frame. Before 
    // sending the image to the controller display_flush() reduces it to 4bpp 
    // and rotates it. ~2.6MB buffer in SPIRAM
    EXT_RAM_BSS_ATTR static uint8_t draw_buff[2][DISPLAY_DRAW_BUF_SIZE] __attribute__((aligned(64)));

    lv_init();
    lv_tick_set_cb(lv_tick_ms);
    // Create the display and attach the displaying function
    lv_display_t *disp = lv_display_create(DISPLAY_UI_W, DISPLAY_UI_H);
    // display_flush() expects L8
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_L8);
    lv_display_set_flush_cb(disp, display_flush);
    lv_display_add_event_cb(disp, display_rounder, LV_EVENT_INVALIDATE_AREA, NULL);
    lv_display_set_buffers(disp, draw_buff[0], draw_buff[1], sizeof(draw_buff[0]), LV_DISPLAY_RENDER_MODE_PARTIAL);
    // Create the UI
    ui_init();

    TickType_t last_battery_check = xTaskGetTickCount();
    ui_set_battery(battery_get_percent());

    // Enter a 10ms background loop (this is not required if the setup is complete)
    while(true) {
        // TODO: Sleep management https://docs.lvgl.io/master/porting/sleep.html
        vTaskDelay(pdMS_TO_TICKS(10));
        // All UI operations must be called from the thread this funciton is 
        // periodically called from.
        lv_timer_handler();

        ui_update(NULL);

        if(xTaskGetTickCount() - last_battery_check >= pdMS_TO_TICKS(BATTERY_PERIOD_MS)) {
            last_battery_check = xTaskGetTickCount();
            const uint8_t percent = battery_get_percent();
            ui_set_battery(percent);
            ble_set_battery_level(percent);
        }
    }
}

// TODO: Could merge the ext_montserrat_14 and the lv_montserrat_14 (to get the
// extra icon glyps) to save some flash
void app_main(void) {
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

    battery_init();
    ble_init();
    display_init();

    if(xTaskCreate(ui_task, "ui", UI_TASK_STACK_SIZE, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE("main", "Failed to create the UI task");
    }
}
