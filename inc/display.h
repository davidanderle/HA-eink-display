#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_HOR_RES (1872)
#define DISPLAY_VER_RES (1404)

void display_init(void);
void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_rounder(lv_event_t *e);

#endif
