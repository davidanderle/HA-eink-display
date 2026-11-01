#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "display.h"

#include "ui_helpers.h"
#include "ui_events.h"

// Only two fonts are available at the moment. Swap them here once more sizes
// are generated
#define UI_FONT_LARGE (&ext_montserrat_48)
#define UI_FONT_SMALL (&ext_montserrat_14)

#define UI_DAYS      (8)  // Rows of the calendar, the first one being today
#define UI_PERSONS   (2)  // Event lines per day
#define UI_CHIPS     (3)  // Event chips per person and day
#define UI_CHORES    (3)
#define UI_PLAN_ROOMS (4)

typedef enum {
    UI_ICON_NONE,
    UI_ICON_SUN,
    UI_ICON_PARTLY_CLOUDY,
    UI_ICON_HOUSE,
    UI_ICON_DROP,
    UI_ICON_FLAME,
    UI_ICON_BOLT,
} ui_icon_t;

// Energy flow nodes around the house
typedef enum {
    UI_ENERGY_SOLAR,
    UI_ENERGY_GRID,
    UI_ENERGY_HEAT,
    UI_ENERGY_WATER,
    UI_ENERGY_COUNT,
} ui_energy_t;

void ui_screen_main_init(void);

extern lv_obj_t * ui_screen_main;
extern lv_obj_t * ui_label_message;

// Icons are drawn with LVGL primitives, so they cost no flash and scale freely
lv_obj_t * ui_icon_create(lv_obj_t * parent, ui_icon_t type, int32_t size);
void ui_icon_set(lv_obj_t * icon, ui_icon_t type);

void ui_set_battery(uint8_t percent);
void ui_set_weather(ui_icon_t icon);
void ui_set_steps(int person, uint32_t steps);
void ui_set_chore(int index, const char * text);
void ui_set_energy(ui_energy_t node, const char * text);
void ui_set_room(int room, bool active);
/// @brief Sets a calendar row (0 is today)
void ui_set_day(int row, ui_icon_t icon, const char * temp, const char * weekday, const char * date);
/// @brief Sets an event chip of a calendar row. A NULL text hides the chip
void ui_set_event(int row, int person, int slot, const char * text);

void ui_init(void);
void ui_update(void *param);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
