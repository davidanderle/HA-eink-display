#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#include "ui_helpers.h"
#include "ui_events.h"

void ui_screen_main_init(void);

extern lv_obj_t * ui_screen_main;
extern lv_obj_t * ui_label_message;

LV_IMG_DECLARE(ui_img_hand_png);    // assets/hand.png

void ui_init(void);
void ui_update(void *param);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
