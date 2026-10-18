/** 无尽版专用 UI；不修改三关版的主题或共用游戏素材。 */
#ifndef SAUSAGE_UI_H
#define SAUSAGE_UI_H

#include "ui/ui_theme.h"

#define SAUSAGE_UI_BG    0x081923
#define SAUSAGE_UI_CELL_A 0x0D2633
#define SAUSAGE_UI_CELL_B 0x123241
#define SAUSAGE_UI_JADE  0x66C7B5
#define SAUSAGE_UI_EDGE  0x244D58
#define SAUSAGE_UI_GOLD  0xF3C04F
#define SAUSAGE_UI_TEXT  0xF4EDD6
#define SAUSAGE_UI_DIM   0x8DA5AD

void sausage_ui_background(lv_obj_t* parent);
lv_obj_t* sausage_ui_doe(lv_obj_t* parent, int32_t scale);
void sausage_ui_screen(lv_obj_t* scr, bool checker);
lv_obj_t* sausage_ui_panel(lv_obj_t* parent, int32_t w, int32_t h);
lv_obj_t* sausage_ui_option(lv_obj_t* parent, const char* text, const lv_font_t* font, int32_t w, int32_t h);
void sausage_ui_focus(lv_obj_t* option, bool focused);
lv_obj_t* sausage_ui_keycap(lv_obj_t* parent, char key);
void sausage_ui_hint(lv_obj_t* parent, const char* text);
lv_obj_t* sausage_ui_keys(lv_obj_t* parent, const char* text);

#endif
