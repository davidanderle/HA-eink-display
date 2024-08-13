#ifndef DISPLAY_H
#define DISPLAY_H

void display_init(void);
void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

#endif
