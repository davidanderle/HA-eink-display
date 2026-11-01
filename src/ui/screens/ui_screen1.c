#include "../ui.h"

// Layout of the 1404x1872 portrait UI (see example_layout.jpg)
#define MARGIN      (32)
#define GAP         (12)
#define CONTENT_W   (DISPLAY_UI_W - 2*MARGIN)
#define CARD_RADIUS (28)

#define ROW1_Y      (16)
#define ROW1_H      (312)
#define DIVIDER1_Y  (348)
#define ROW2_Y      (372)
#define ROW2_H      (186)
#define DIVIDER2_Y  (578)
#define CAL_Y       (598)
#define CAL_ROW_H   (136)
#define CAL_PITCH   (146)
#define CAL_COL1_W  (130)
#define CAL_COL2_W  (134)
#define CAL_EVENTS_X (CAL_COL1_W + CAL_COL2_W + 16)
#define CHIP_H      (56)
#define BAR_Y       (CAL_Y + UI_DAYS*CAL_PITCH + 20)
#define BAR_H       (56)

// 4bpp panel: keep the greys at least 0x10 apart
#define GREY_LIGHT  (0xEE)
#define GREY_DARK   (0xDD)
#define GREY_CHIP   (0xAA)
#define GREY_PILL   (0x99)
#define GREY_LINE   (0x80)

lv_obj_t * ui_screen_main;
lv_obj_t * ui_label_message;

static lv_obj_t * battery_fill;
static lv_obj_t * weather_icon;
static lv_obj_t * steps_label[UI_PERSONS];
static lv_obj_t * chore_label[UI_CHORES];
static lv_obj_t * energy_label[UI_ENERGY_COUNT];
static lv_obj_t * room_marker[UI_PLAN_ROOMS];

static struct {
    lv_obj_t * icon;
    lv_obj_t * temp;
    lv_obj_t * weekday;
    lv_obj_t * date;
    lv_obj_t * chip[UI_PERSONS][UI_CHIPS];
    lv_obj_t * chip_label[UI_PERSONS][UI_CHIPS];
} day[UI_DAYS];

static lv_obj_t * box(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t grey, int32_t radius) {
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_make(grey, grey, grey), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    return o;
}

static lv_obj_t * transparent_box(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h) {
    lv_obj_t * o = box(parent, x, y, w, h, 0xFF, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    return o;
}

static lv_obj_t * label(lv_obj_t * parent, const lv_font_t * font, int32_t x, int32_t y, int32_t w, lv_text_align_t align) {
    lv_obj_t * l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_black(), 0);
    lv_obj_set_style_text_align(l, align, 0);
    return l;
}

static void outline(lv_obj_t * o, int32_t width, int32_t radius) {
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, width, 0);
    lv_obj_set_style_border_color(o, lv_color_black(), 0);
    lv_obj_set_style_radius(o, radius, 0);
}

static lv_obj_t * card(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t grey) {
    return box(ui_screen_main, x, y, w, h, grey, CARD_RADIUS);
}

static void titled_card_header(lv_obj_t * card, const char * title, int32_t y_line) {
    if(title) {
        lv_obj_t * t = label(card, UI_FONT_LARGE, 0, 6, lv_obj_get_width(card), LV_TEXT_ALIGN_CENTER);
        lv_label_set_text(t, title);
    }
    box(card, 24, y_line, lv_obj_get_width(card) - 48, 6, GREY_LINE, 3);
}

static void build_floor_plan(void) {
    lv_obj_t * c = card(MARGIN, ROW1_Y, 388, ROW1_H, GREY_LIGHT);
    lv_obj_t * frame = transparent_box(c, 28, 24, 332, 264);
    outline(frame, 6, 0);
    static const lv_point_t pos[UI_PLAN_ROOMS] = { {24, 24}, {236, 24}, {24, 180}, {236, 196} };
    for(int i = 0; i < UI_PLAN_ROOMS; i++) {
        lv_obj_t * m = transparent_box(frame, pos[i].x, pos[i].y, 52, 52);
        outline(m, 5, 8);
        room_marker[i] = m;
    }
}

static void build_chores(void) {
    lv_obj_t * c = card(MARGIN + 388 + GAP, ROW1_Y, 540, ROW1_H, GREY_DARK);
    titled_card_header(c, "Chores", 74);
    for(int i = 0; i < UI_CHORES; i++) {
        chore_label[i] = label(c, UI_FONT_LARGE, 28, 90 + i*64, 484, LV_TEXT_ALIGN_LEFT);
    }
}

static void build_energy(void) {
    const int32_t x = MARGIN + 388 + GAP + 540 + GAP;
    lv_obj_t * c = card(x, ROW1_Y, 388, ROW1_H, GREY_LIGHT);
    const int32_t N = 80;       // node diameter
    const int32_t cx = 194, cy = 156;
    // Node centres: solar (top), grid (left), heat (right), water (bottom)
    static const lv_point_t off[UI_ENERGY_COUNT] = { {0, -108}, {-136, 0}, {136, 0}, {0, 108} };
    static const ui_icon_t icons[UI_ENERGY_COUNT] = { UI_ICON_SUN, UI_ICON_BOLT, UI_ICON_FLAME, UI_ICON_DROP };

    // Connectors first, so that they sit behind the nodes
    box(c, cx - 2, cy - 108, 4, 216, GREY_LINE, 0);
    box(c, cx - 136, cy - 2, 272, 4, GREY_LINE, 0);

    lv_obj_t * center = box(c, cx - N/2, cy - N/2, N, N, 0xFF, LV_RADIUS_CIRCLE);
    outline(center, 5, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(center, lv_color_white(), 0);
    lv_obj_align(ui_icon_create(center, UI_ICON_HOUSE, 54), LV_ALIGN_CENTER, 0, 0);

    for(int i = 0; i < UI_ENERGY_COUNT; i++) {
        const int32_t nx = cx + off[i].x - N/2, ny = cy + off[i].y - N/2;
        lv_obj_t * n = box(c, nx, ny, N, N, 0xFF, LV_RADIUS_CIRCLE);
        outline(n, 5, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(n, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(n, lv_color_white(), 0);
        lv_obj_align(ui_icon_create(n, icons[i], 54), LV_ALIGN_CENTER, 0, 0);

        // Top and bottom values sit next to the node, left and right ones below
        const bool vertical = (i == UI_ENERGY_SOLAR || i == UI_ENERGY_WATER);
        energy_label[i] = vertical ?
            label(c, UI_FONT_SMALL, nx + N + 12, ny + N/2 - 10, 120, LV_TEXT_ALIGN_LEFT) :
            label(c, UI_FONT_SMALL, nx - 6, ny + N + 8, N + 12, LV_TEXT_ALIGN_CENTER);
    }
}

static void build_row2(void) {
    // Step count
    lv_obj_t * steps = card(MARGIN, ROW2_Y, 366, ROW2_H, GREY_DARK);
    titled_card_header(steps, "Step Count", 62);
    for(int i = 0; i < UI_PERSONS; i++) {
        steps_label[i] = label(steps, UI_FONT_LARGE, 24, 68 + i*56, 318, LV_TEXT_ALIGN_LEFT);
    }

    // Free slot
    card(MARGIN + 366 + GAP, ROW2_Y, 262, ROW2_H, GREY_LIGHT);

    // Current weather
    lv_obj_t * weather = card(MARGIN + 366 + GAP + 262 + GAP, ROW2_Y, 262, ROW2_H, GREY_LIGHT);
    weather_icon = ui_icon_create(weather, UI_ICON_NONE, 150);
    lv_obj_align(weather_icon, LV_ALIGN_CENTER, 0, 0);

    // Free slot with a header line
    lv_obj_t * info = card(MARGIN + 366 + GAP + 262 + GAP + 262 + GAP, ROW2_Y, 414, ROW2_H, GREY_DARK);
    titled_card_header(info, NULL, 56);
}

static lv_obj_t * chip_create(lv_obj_t * parent, lv_obj_t ** out_label) {
    lv_obj_t * chip = box(parent, 0, 0, LV_SIZE_CONTENT, CHIP_H, GREY_CHIP, LV_RADIUS_CIRCLE);
    lv_obj_set_style_pad_hor(chip, 22, 0);
    lv_obj_t * l = lv_label_create(chip);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_max_width(l, 290, 0);
    lv_obj_set_style_text_font(l, UI_FONT_LARGE, 0);
    lv_obj_set_style_text_color(l, lv_color_black(), 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_hidden(chip, true);
    *out_label = l;
    return chip;
}

static void build_calendar(void) {
    for(int r = 0; r < UI_DAYS; r++) {
        const int32_t y = CAL_Y + r*CAL_PITCH;
        lv_obj_t * row = box(ui_screen_main, MARGIN, y, CONTENT_W, CAL_ROW_H, (r & 1) ? GREY_DARK : GREY_LIGHT, CARD_RADIUS);

        day[r].icon = ui_icon_create(row, UI_ICON_NONE, 64);
        lv_obj_align(day[r].icon, LV_ALIGN_TOP_LEFT, (CAL_COL1_W - 64)/2, 14);
        day[r].temp = label(row, UI_FONT_SMALL, 0, 94, CAL_COL1_W, LV_TEXT_ALIGN_CENTER);
        box(row, CAL_COL1_W, 18, 4, CAL_ROW_H - 36, GREY_LINE, 2);

        day[r].weekday = label(row, UI_FONT_LARGE, CAL_COL1_W + 4, 10, CAL_COL2_W, LV_TEXT_ALIGN_CENTER);
        day[r].date = label(row, UI_FONT_LARGE, CAL_COL1_W + 4, 70, CAL_COL2_W, LV_TEXT_ALIGN_CENTER);
        box(row, CAL_COL1_W + CAL_COL2_W + 4, 18, 4, CAL_ROW_H - 36, GREY_LINE, 2);

        const int32_t ev_w = CONTENT_W - CAL_EVENTS_X - 20;
        box(row, CAL_EVENTS_X, CAL_ROW_H/2 - 2, ev_w, 4, GREY_LINE, 2);
        for(int p = 0; p < UI_PERSONS; p++) {
            lv_obj_t * lane = transparent_box(row, CAL_EVENTS_X, p * CAL_ROW_H/2, ev_w, CAL_ROW_H/2);
            lv_obj_set_flex_flow(lane, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(lane, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(lane, 16, 0);
            for(int s = 0; s < UI_CHIPS; s++) {
                day[r].chip[p][s] = chip_create(lane, &day[r].chip_label[p][s]);
            }
        }
    }
}

static void build_status_bar(void) {
    lv_obj_t * pill = box(ui_screen_main, MARGIN, BAR_Y, 740, BAR_H, GREY_PILL, LV_RADIUS_CIRCLE);
    ui_label_message = label(pill, UI_FONT_SMALL, 28, (BAR_H - 21)/2, 684, LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(ui_label_message, "");

    // Battery: outline, terminal nub and a fill proportional to the charge
    const int32_t bw = 120, right = DISPLAY_UI_W - MARGIN;
    lv_obj_t * body = transparent_box(ui_screen_main, right - 12 - bw, BAR_Y, bw, BAR_H);
    outline(body, 5, 10);
    box(ui_screen_main, right - 12, BAR_Y + BAR_H/2 - 13, 12, 26, 0x00, 4);
    battery_fill = box(body, 8, 8, bw - 26, BAR_H - 26, 0x00, 4);
}

// Placeholder content mirroring the sketch until the data source is connected
static void load_sample_data(void) {
    ui_set_weather(UI_ICON_PARTLY_CLOUDY);
    ui_set_steps(0, 2500);
    ui_set_steps(1, 2700);
    ui_set_chore(0, "Bins");
    ui_set_chore(1, "Laundry");
    ui_set_chore(2, "Plants");
    ui_set_energy(UI_ENERGY_SOLAR, "2.3 kW");
    ui_set_energy(UI_ENERGY_GRID, "-1.6 kW");
    ui_set_energy(UI_ENERGY_HEAT, "6 kW");
    ui_set_energy(UI_ENERGY_WATER, "200 L");
    ui_set_room(0, true);
    ui_set_room(1, true);
    ui_set_room(2, true);
    static const char * const weekdays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon" };
    for(int r = 0; r < UI_DAYS; r++) {
        char date[4];
        lv_snprintf(date, sizeof(date), "%d", 14 + r);
        ui_set_day(r, r % 3 == 1 ? UI_ICON_PARTLY_CLOUDY : UI_ICON_SUN, "21\xC2\xB0""C", weekdays[r], date);
    }
    ui_set_event(0, 0, 0, "Dentist");
    ui_set_event(0, 0, 1, "Gym");
    ui_set_event(0, 1, 0, "Work");
    ui_set_event(2, 1, 0, "Dinner");
}

// Note: observer/subject could be used if multiple UI elements depend on a
// single variable change
void ui_screen_main_init(void) {
    ui_screen_main = lv_obj_create(NULL);
    lv_obj_set_scrollable(ui_screen_main, false);
    lv_obj_set_style_bg_color(ui_screen_main, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(ui_screen_main, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(ui_screen_main, 0, 0);

    build_floor_plan();
    build_chores();
    build_energy();
    box(ui_screen_main, MARGIN, DIVIDER1_Y, CONTENT_W, 6, 0x00, 3);
    build_row2();
    box(ui_screen_main, MARGIN, DIVIDER2_Y, CONTENT_W, 6, 0x00, 3);
    build_calendar();
    build_status_bar();

    load_sample_data();
}

void ui_set_battery(uint8_t percent) {
    percent = LV_MIN(percent, 100);
    lv_obj_set_width(battery_fill, LV_MAX(1, (120 - 26) * percent / 100));
}

void ui_set_weather(ui_icon_t icon) {
    ui_icon_set(weather_icon, icon);
}

void ui_set_steps(int person, uint32_t steps) {
    if(person < 0 || person >= UI_PERSONS) return;
    lv_label_set_text_fmt(steps_label[person], "%" LV_PRIu32, steps);
}

void ui_set_chore(int index, const char * text) {
    if(index < 0 || index >= UI_CHORES) return;
    lv_label_set_text(chore_label[index], text ? text : "");
}

void ui_set_energy(ui_energy_t node, const char * text) {
    if(node >= UI_ENERGY_COUNT) return;
    lv_label_set_text(energy_label[node], text ? text : "");
}

void ui_set_room(int room, bool active) {
    if(room < 0 || room >= UI_PLAN_ROOMS) return;
    lv_obj_set_style_bg_opa(room_marker[room], active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(room_marker[room], lv_color_black(), 0);
}

void ui_set_day(int row, ui_icon_t icon, const char * temp, const char * weekday, const char * date) {
    if(row < 0 || row >= UI_DAYS) return;
    ui_icon_set(day[row].icon, icon);
    lv_label_set_text(day[row].temp, temp ? temp : "");
    lv_label_set_text(day[row].weekday, weekday ? weekday : "");
    lv_label_set_text(day[row].date, date ? date : "");
}

void ui_set_event(int row, int person, int slot, const char * text) {
    if(row < 0 || row >= UI_DAYS || person < 0 || person >= UI_PERSONS || slot < 0 || slot >= UI_CHIPS) return;
    if(text == NULL) {
        lv_obj_set_hidden(day[row].chip[person][slot], true);
        return;
    }
    lv_label_set_text(day[row].chip_label[person][slot], text);
    lv_obj_set_hidden(day[row].chip[person][slot], false);
}
