/**
 * 贪吃腊肠共用控件
 */

#include "snake_ui.h"
#include "snake_art.h"
#include "ui/ui_theme.h"

#define OPTION_BORDER  2
#define OPTION_RADIUS  4
#define POINTER_SCALE  2
#define POINTER_PX     (SNAKE_ARROW * POINTER_SCALE)
#define POINTER_INSET  8

// 选项的子对象：文字、指针
#define CHILD_LABEL   0
#define CHILD_POINTER 1

static lv_draw_buf_t* s_pointer_buf;

lv_obj_t* snake_option_create(lv_obj_t* parent, const char* text, const lv_font_t* font, int32_t w, int32_t h) {
    lv_obj_t* option = ui_box_create(parent);
    lv_obj_set_size(option, w, h);
    lv_obj_set_style_bg_color(option, lv_color_hex(UI_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(option, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(option, OPTION_RADIUS, 0);
    lv_obj_set_style_border_width(option, OPTION_BORDER, 0);

    lv_obj_t* label = lv_label_create(option);
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    lv_obj_t* pointer = snake_icon_create(option, &s_pointer_buf, &snake_arrow, POINTER_SCALE);
    lv_obj_align(pointer, LV_ALIGN_LEFT_MID, POINTER_INSET - OPTION_BORDER, 0);

    snake_option_set_focused(option, false);
    return option;
}

void snake_option_set_focused(lv_obj_t* option, bool focused) {
    uint32_t color = focused ? UI_COLOR_FOCUS : UI_COLOR_LINE_DIM;
    lv_obj_set_style_border_color(option, lv_color_hex(color), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(option, CHILD_LABEL),
                                lv_color_hex(focused ? UI_COLOR_FOCUS : UI_COLOR_TEXT), 0);
    lv_obj_set_hidden(lv_obj_get_child(option, CHILD_POINTER), !focused);
}

lv_obj_t* snake_icon_create(lv_obj_t* parent, lv_draw_buf_t** buf, const Sprite* sprite, int32_t scale) {
    sprite_buf_render(buf, sprite, 1, (SpriteTurn){0, false});
    lv_obj_t* icon = lv_image_create(parent);
    lv_image_set_src(icon, *buf);
    lv_image_set_antialias(icon, false);
    lv_image_set_inner_align(icon, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_set_size(icon, sprite->h * scale, sprite->h * scale);
    return icon;
}
