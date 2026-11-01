#ifndef DISPLAY_H
#define DISPLAY_H

// Native (landscape) resolution of the panel
#define DISPLAY_HOR_RES (1872)
#define DISPLAY_VER_RES (1404)
// The panel is mounted in portrait: LVGL renders a 1404x1872 UI and
// display_flush() rotates it to the panel's native orientation
#define DISPLAY_UI_W (DISPLAY_VER_RES)
#define DISPLAY_UI_H (DISPLAY_HOR_RES)
// LVGL renders in L8 (1 byte per pixel), in two buffers of half a frame each
#define DISPLAY_DRAW_BUF_SIZE ((DISPLAY_UI_W*DISPLAY_UI_H)/2)

void display_init(void);
void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_rounder(lv_event_t *e);

#endif
