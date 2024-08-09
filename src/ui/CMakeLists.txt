SET(SOURCES screens/ui_screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_hand_png.c)

add_library(ui ${SOURCES})
