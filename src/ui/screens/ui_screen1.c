#include "../ui.h"

// Note: observer/subject could be used if multiple UI elements depend on a
// single variable change
void ui_screen_main_init(void) {
    ui_screen_main = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_screen_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_image_src(ui_screen_main, &ui_img_hand_png, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_label_message = lv_label_create(ui_screen_main);
    lv_obj_set_align(ui_label_message, LV_ALIGN_CENTER);
    lv_label_set_long_mode(ui_label_message, LV_LABEL_LONG_DOT);
    lv_label_set_text(ui_label_message, "\xF0\x9F\xA4\x96 \xE2\x99\xA5 \xF0\x9F\x8E\x83 \xF0\x9F\xA5\xB0");
    lv_obj_set_style_text_font(ui_label_message, &ext_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
}
