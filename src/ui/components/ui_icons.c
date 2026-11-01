#include "../ui.h"

#define ICON_COLOR lv_color_hex(0x303030)

static void line(lv_layer_t * layer, int32_t w, float x1, float y1, float x2, float y2) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = ICON_COLOR;
    dsc.width = w;
    dsc.round_start = dsc.round_end = 1;
    dsc.p1.x = x1; dsc.p1.y = y1;
    dsc.p2.x = x2; dsc.p2.y = y2;
    lv_draw_line(layer, &dsc);
}

// Circle with an optional fill; border width 0 draws a solid disc
static void circle(lv_layer_t * layer, float cx, float cy, float r, int32_t border, lv_color_t fill, bool filled) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_opa = filled ? LV_OPA_COVER : LV_OPA_TRANSP;
    dsc.bg_color = fill;
    dsc.border_width = border;
    dsc.border_color = ICON_COLOR;
    dsc.border_opa = border ? LV_OPA_COVER : LV_OPA_TRANSP;
    const lv_area_t a = { (int32_t)(cx - r), (int32_t)(cy - r), (int32_t)(cx + r), (int32_t)(cy + r) };
    lv_draw_rect(layer, &dsc, &a);
}

static void tri(lv_layer_t * layer, float x1, float y1, float x2, float y2, float x3, float y3) {
    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.color = ICON_COLOR;
    dsc.p[0].x = x1; dsc.p[0].y = y1;
    dsc.p[1].x = x2; dsc.p[1].y = y2;
    dsc.p[2].x = x3; dsc.p[2].y = y3;
    lv_draw_triangle(layer, &dsc);
}

static void sun(lv_layer_t * layer, float cx, float cy, float s, int32_t lw) {
    static const float dir[8][2] = {
        {1, 0}, {0.7071f, 0.7071f}, {0, 1}, {-0.7071f, 0.7071f},
        {-1, 0}, {-0.7071f, -0.7071f}, {0, -1}, {0.7071f, -0.7071f},
    };
    circle(layer, cx, cy, s * 0.2f, lw, lv_color_white(), false);
    for(int i = 0; i < 8; i++) {
        line(layer, lw, cx + dir[i][0] * s * 0.32f, cy + dir[i][1] * s * 0.32f,
                        cx + dir[i][0] * s * 0.46f, cy + dir[i][1] * s * 0.46f);
    }
}

static void icon_draw_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target_obj(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    const float s = lv_area_get_width(&a);
    const float x = a.x1, y = a.y1;
    const int32_t lw = LV_MAX(3, (int32_t)(s / 16));
    const lv_color_t ink = ICON_COLOR;

    switch((ui_icon_t)(uintptr_t)lv_obj_get_user_data(obj)) {
    case UI_ICON_SUN:
        sun(layer, x + s * 0.5f, y + s * 0.5f, s, lw);
        break;
    case UI_ICON_PARTLY_CLOUDY:
        sun(layer, x + s * 0.34f, y + s * 0.34f, s * 0.7f, lw);
        circle(layer, x + s * 0.40f, y + s * 0.72f, s * 0.14f, 0, ink, true);
        circle(layer, x + s * 0.58f, y + s * 0.62f, s * 0.20f, 0, ink, true);
        circle(layer, x + s * 0.78f, y + s * 0.74f, s * 0.13f, 0, ink, true);
        line(layer, (int32_t)(s * 0.13f), x + s * 0.40f, y + s * 0.82f, x + s * 0.78f, y + s * 0.82f);
        break;
    case UI_ICON_HOUSE:
        line(layer, lw, x + s * 0.12f, y + s * 0.48f, x + s * 0.5f, y + s * 0.14f);
        line(layer, lw, x + s * 0.5f, y + s * 0.14f, x + s * 0.88f, y + s * 0.48f);
        line(layer, lw, x + s * 0.24f, y + s * 0.44f, x + s * 0.24f, y + s * 0.84f);
        line(layer, lw, x + s * 0.76f, y + s * 0.44f, x + s * 0.76f, y + s * 0.84f);
        line(layer, lw, x + s * 0.24f, y + s * 0.84f, x + s * 0.76f, y + s * 0.84f);
        break;
    case UI_ICON_DROP:
    case UI_ICON_FLAME:
        tri(layer, x + s * 0.5f, y + s * 0.1f, x + s * 0.24f, y + s * 0.6f, x + s * 0.76f, y + s * 0.6f);
        circle(layer, x + s * 0.5f, y + s * 0.64f, s * 0.26f, 0, ink, true);
        if(lv_obj_get_user_data(obj) == (void *)UI_ICON_FLAME) {
            circle(layer, x + s * 0.5f, y + s * 0.7f, s * 0.1f, 0, lv_color_white(), true);
        }
        break;
    case UI_ICON_BOLT:
        tri(layer, x + s * 0.6f, y + s * 0.08f, x + s * 0.28f, y + s * 0.56f, x + s * 0.52f, y + s * 0.56f);
        tri(layer, x + s * 0.48f, y + s * 0.44f, x + s * 0.72f, y + s * 0.44f, x + s * 0.4f, y + s * 0.92f);
        break;
    default:
        break;
    }
}

lv_obj_t * ui_icon_create(lv_obj_t * parent, ui_icon_t type, int32_t size) {
    lv_obj_t * obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_user_data(obj, (void *)(uintptr_t)type);
    lv_obj_add_event_cb(obj, icon_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    return obj;
}

void ui_icon_set(lv_obj_t * icon, ui_icon_t type) {
    if((ui_icon_t)(uintptr_t)lv_obj_get_user_data(icon) == type) return;
    lv_obj_set_user_data(icon, (void *)(uintptr_t)type);
    lv_obj_invalidate(icon);
}
