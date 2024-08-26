#include "ui.h"
#include "ui_helpers.h"
#include "ble.h"
#include "esp_log.h"

const char *tag = "UI";

void ui_screen_main_init(void);

lv_obj_t * ui_screen_main;
lv_obj_t * ui_label_message;

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif

extern volatile bool is_json_modified;
extern const char *json_path;
extern SemaphoreHandle_t file_mutex;

/// @brief This function should be called periodically from the same thread as
/// the lv_timer_handler() is being called from. It ensures that the values from
/// the MVP model are safely updated in the UI (View)
void ui_update(void *param) {
    ESP_UNUSED(param);

    if(is_json_modified && xSemaphoreTake(file_mutex, 1)) {
        ESP_LOGI(tag, "Detected modified ui file");
        FILE *f = fopen(json_path, "r");
        if(f == NULL) {
            ESP_LOGE(tag, "Failed to open the file for reading: %s", json_path);
            xSemaphoreGive(file_mutex);
            return;
        }

        ESP_LOGI(tag, "Attempting to read the file's content");
        EXT_RAM_BSS_ATTR static char buff[MAX_BLE_MSG_SIZE];
        fseek(f, 0, SEEK_SET);
        const size_t read_bytes = fread(buff, 1, sizeof(buff)-1, f);
        fclose(f);
        xSemaphoreGive(file_mutex);

        ESP_LOGI(tag, "Read %u bytes from file", read_bytes);

        is_json_modified = false;
        if(read_bytes > 0){
            buff[read_bytes] = '\0';
            ESP_LOGI(tag, "Read data: %s", buff);
            if(strcmp(lv_label_get_text(ui_label_message), buff) != 0) {
                ESP_LOGI(tag, "Updating UI with new data...");
                lv_label_set_text(ui_label_message, buff);
            }
        }
    }
}

void ui_init(void) {
    ui_screen_main_init();
    lv_disp_load_scr(ui_screen_main);
}
