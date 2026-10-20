#include "sausage_ui.h"
#include "snake_ui.h"
#include "snake_art.h"
#include <string.h>

void sausage_ui_background(lv_obj_t* parent) {
    lv_obj_set_style_bg_color(parent, lv_color_hex(SAUSAGE_UI_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    // 用原生像素矩形铺底，不引入图片或改变游戏精灵。
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 12; x++) {
            lv_obj_t* cell = ui_box_create(parent);
            lv_obj_set_pos(cell, x * 20, y * 20);
            lv_obj_set_size(cell, 20, 20);
            lv_obj_set_style_bg_color(cell, lv_color_hex((x + y) % 2 ? SAUSAGE_UI_CELL_B : SAUSAGE_UI_CELL_A), 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        }
    }
    lv_obj_t* frame = ui_box_create(parent);
    lv_obj_set_pos(frame, 2, 2);
    lv_obj_set_size(frame, 236, 236);
    lv_obj_set_style_radius(frame, 2, 0);
    lv_obj_set_style_border_width(frame, 1, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(SAUSAGE_UI_JADE), 0);
}

// 页面展示图仅拼接现有角色部件，不改变局内角色资产或格子绘制。
lv_obj_t* sausage_ui_doe(lv_obj_t* parent, int32_t scale) {
    static lv_draw_buf_t* buf;
    const int inset = (SNAKE_LEGS - SNAKE_TILE) / 2;
    const int width = 4 * SNAKE_TILE + 2 * inset;
    if (!buf) {
        sprite_buf_begin(&buf, width, SNAKE_LEGS);
        sprite_buf_paint_at(buf, &snake_tail, inset, inset);
        sprite_buf_paint_at(buf, &snake_body, inset + SNAKE_TILE, inset);
        sprite_buf_paint_at(buf, &snake_body, inset + 2 * SNAKE_TILE, inset);
        sprite_buf_paint_at(buf, &snake_head, inset + 3 * SNAKE_TILE, inset);
        sprite_buf_paint_at(buf, &snake_back_legs[0], 0, 0);
        sprite_buf_paint_at(buf, &snake_front_legs[0], 3 * SNAKE_TILE, 0);
    }
    lv_obj_t* doe = lv_image_create(parent);
    lv_image_set_src(doe, buf);
    lv_image_set_antialias(doe, false);
    lv_image_set_inner_align(doe, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_set_size(doe, width * scale, SNAKE_LEGS * scale);
    return doe;
}

void sausage_ui_screen(lv_obj_t* scr, bool checker) {
    ui_screen_dark(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(SAUSAGE_UI_BG), 0);
    if (checker) sausage_ui_background(scr);
}

lv_obj_t* sausage_ui_panel(lv_obj_t* parent, int32_t w, int32_t h) {
    lv_obj_t* panel = ui_box_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_bg_color(panel, lv_color_hex(SAUSAGE_UI_BG), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(SAUSAGE_UI_JADE), 0);
    lv_obj_set_style_radius(panel, 4, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    return panel;
}

void sausage_ui_focus(lv_obj_t* option, bool focused) {
    snake_option_set_focused(option, focused);
    lv_obj_set_style_border_color(option, lv_color_hex(focused ? SAUSAGE_UI_GOLD : SAUSAGE_UI_DIM), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(option, 0), lv_color_hex(SAUSAGE_UI_TEXT), 0);
}

lv_obj_t* sausage_ui_option(lv_obj_t* parent, const char* text, const lv_font_t* font, int32_t w, int32_t h) {
    lv_obj_t* option = snake_option_create(parent, text, font, w, h);
    lv_obj_set_style_bg_color(option, lv_color_hex(SAUSAGE_UI_BG), 0);
    lv_obj_set_style_border_width(option, 1, 0);
    lv_obj_set_style_radius(option, 2, 0);
    lv_obj_set_overflow_visible(option, true);
    lv_obj_align(lv_obj_get_child(option, 1), LV_ALIGN_OUT_LEFT_MID, -6, 0);
    sausage_ui_focus(option, false);
    return option;
}

// 与实机对应：选择白色、确认蓝色、返回黄色；字母仅供内部映射。
lv_obj_t* sausage_ui_keycap(lv_obj_t* parent, char key) {
    uint32_t color = key == 'B' ? 0x4B9CFF : key == 'C' ? 0xF3C04F : 0xFFFFFF;
    lv_obj_t* cap = ui_box_create(parent);
    lv_obj_set_size(cap, 16, 16);
    lv_obj_set_style_bg_color(cap, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(cap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cap, 2, 0);
    lv_obj_set_style_border_color(cap, lv_color_hex(color), 0);
    lv_obj_set_style_radius(cap, 2, 0);
    // 内凹的深色轮廓让小色块看起来像实体按键。
    lv_obj_t* face = ui_box_create(cap);
    lv_obj_set_size(face, 8, 8);
    lv_obj_set_style_border_width(face, 1, 0);
    lv_obj_set_style_border_color(face, lv_color_hex(SAUSAGE_UI_BG), 0);
    lv_obj_center(face);
    return cap;
}

lv_obj_t* sausage_ui_keys(lv_obj_t* parent, const char* text) {
    lv_obj_t* row = ui_box_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 6, 0);
    const char* next = text;
    while (*next) {
        while (*next == ' ') next++;
        if (!*next) break;
        char key[2] = {*next++, '\0'};
        while (*next == ' ') next++;
        const char* end = strstr(next, "  ");
        if (!end) end = next + strlen(next);
        lv_obj_t* group = ui_box_create(row);
        lv_obj_set_size(group, LV_SIZE_CONTENT, 16);
        lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(group, 3, 0);
        sausage_ui_keycap(group, key[0]);
        lv_obj_t* action = lv_label_create(group);
        lv_obj_set_style_text_font(action, &font_sausage_12, 0);
        lv_obj_set_style_text_color(action, lv_color_hex(SAUSAGE_UI_TEXT), 0);
        char action_text[64];
        size_t n = (size_t)(end - next);
        if (n >= sizeof(action_text)) n = sizeof(action_text) - 1;
        memcpy(action_text, next, n);
        action_text[n] = '\0';
        lv_label_set_text(action, action_text);
        next = end;
    }
    return row;
}

void sausage_ui_hint(lv_obj_t* parent, const char* text) {
    lv_obj_t* hint = sausage_ui_keys(parent, text);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 212);
}
