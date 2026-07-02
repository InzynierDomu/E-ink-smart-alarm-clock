SET(SOURCES ui_Screen2.c
    ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_rect1_png.c
    fonts/ui_font_Font1.c
    fonts/ui_font_Font2.c
    fonts/ui_font_Font3.c
    fonts/ui_font_weathericon.c
    fonts/ui_font_weathericons.c)

add_library(ui ${SOURCES})
