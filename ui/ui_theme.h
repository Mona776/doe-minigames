/**
 * 界面风格 - 配色和通用控件（面板、标题、按钮、提示条、图标），按 UI 设计稿：深色终端风 + CRT 机壳（crt.h）
 * 像素风：描边 2px（等于背景放大后的 1 个像素），图标放大 2 倍，文字用像素字体（12px 正文、24px 标题）
 * 图标素材：assets/raw/ui/<名字>，没有素材时图标不显示
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <stdbool.h>
#include "lvgl.h"

#define UI_COLOR_SCREEN    0x15161C  // 子页面底色
#define UI_COLOR_PANEL     0x1F212A  // 面板底色
#define UI_COLOR_LINE      0x9CC36B  // 终端绿：面板边框
#define UI_COLOR_LINE_DIM  0x4A6B3A  // 苔绿：分隔线
#define UI_COLOR_GLOW      0xC9F25B  // 毒柠檬：选中角标、时钟数字
#define UI_COLOR_FOCUS     0xF5C242  // 暖琥珀：选中按钮
#define UI_COLOR_CREAM     0xEFE6CF  // 奶油白：普通按钮
#define UI_COLOR_INK       0x1B1C24  // 深空：按钮上的文字和描边
#define UI_COLOR_TEXT      0xEFE6CF
#define UI_COLOR_TEXT_DIM  0x9C968A
#define UI_COLOR_ALERT     0xE0474C  // 信号红

#define UI_ICON_ZOOM 2
#define UI_SAFE_EDGE 12  // CRT 机壳和发光线占掉的边，控件离屏幕边缘至少这么远
#define UI_CONTENT_W (240 - 2 * UI_SAFE_EDGE)

// 没有任何样式的容器
lv_obj_t* ui_box_create(lv_obj_t* parent);

// 深色底 + 终端绿描边
lv_obj_t* ui_panel_create(lv_obj_t* parent);

// 子页面：深色底
void ui_screen_dark(lv_obj_t* scr);

// 顶部标题：两边爪印，中间 24px 标题，下面一条分隔线；宽度 UI_CONTENT_W
lv_obj_t* ui_title_create(lv_obj_t* parent, const char* text);

// 菜单按钮：左边图标、中间 24px 文字、右边箭头；奶油白，选中时琥珀色加四角括号
lv_obj_t* ui_button_create(lv_obj_t* parent, const char* icon, const char* text);
void ui_button_set_focused(lv_obj_t* button, bool focused);

// 底部提示条：深色底、终端绿描边
lv_obj_t* ui_hint_bar_create(lv_obj_t* parent, const char* text);
// 提示条改成提示消息的样子（琥珀底深色字）
void ui_hint_bar_set_alert(lv_obj_t* bar);

// 放大 2 倍显示的图标；name 是 assets/raw/ui/ 下的名字
lv_obj_t* ui_icon_create(lv_obj_t* parent, const char* name);
void ui_icon_set(lv_obj_t* icon, const char* name);

// 一行数值图标：label 加 max 个图标，前 value 个用 <icon>_full，其余 <icon>_empty；icon 要是常量字符串
lv_obj_t* ui_stat_row_create(lv_obj_t* parent, const char* label, const char* icon, int32_t max);
void ui_stat_row_set(lv_obj_t* row, int32_t value);

#endif
