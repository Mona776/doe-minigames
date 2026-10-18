/**
 * 贪吃腊肠几个页面共用的小控件：选项（选中时暖黄边框加左边的指针）、像素图标
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef SNAKE_UI_H
#define SNAKE_UI_H

#include <stdbool.h>
#include "lvgl.h"
#include "ui/sprite.h"

lv_obj_t* snake_option_create(lv_obj_t* parent, const char* text, const lv_font_t* font, int32_t w, int32_t h);
void snake_option_set_focused(lv_obj_t* option, bool focused);

// 一张显示 sprite 的图：*buf 里只存原尺寸（规则同 sprite_buf_render，缓冲一直留着，所以不按放大后的尺寸存），
// 由图片对象按最近邻放大 scale 倍
lv_obj_t* snake_icon_create(lv_obj_t* parent, lv_draw_buf_t** buf, const Sprite* sprite, int32_t scale);

#endif
