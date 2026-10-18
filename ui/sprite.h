/**
 * 像素画精灵 - 用字符画写的小图，按整数倍放大画到 LVGL canvas 上，正式美术到位前占位用
 * 每行一个字符串，'.' 透明，其他字母查 sprite.c 里的调色板
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef SPRITE_H
#define SPRITE_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

typedef struct {
    const char* const* rows;
    uint8_t h;
} Sprite;

#define SPRITE(rows) {(rows), (uint8_t)(sizeof(rows) / sizeof((rows)[0]))}

// buf_slot 指向调用方的静态指针：第一次调用时分配，之后一直复用。
// 页面关闭后旧画面还要在切换动画里画 200ms，所以缓冲不能跟页面一起释放
lv_obj_t* sprite_canvas_create(lv_obj_t* parent, lv_draw_buf_t** buf_slot, int32_t w, int32_t h);

// 清空画布后从左上角画 sprite，每个像素放大成 scale×scale
void sprite_draw(lv_obj_t* canvas, const Sprite* sprite, int32_t scale);

// 朝向变换：先顺时针转 rot 个 90°，再按 mirror 左右翻转；旋转只对正方形的 sprite 有效，不转时任意宽高都能翻转
typedef struct {
    uint8_t rot;
    bool mirror;
} SpriteTurn;

// 同 sprite_draw，按 turn 变换后再画
void sprite_draw_turned(lv_obj_t* canvas, const Sprite* sprite, int32_t scale, SpriteTurn turn);

// 画到一块不挂在画布上的缓冲，可以直接给 lv_image 当图源，多个对象共用一份。buf_slot 的规则同 sprite_canvas_create
lv_draw_buf_t* sprite_buf_render(lv_draw_buf_t** buf_slot, const Sprite* sprite, int32_t scale, SpriteTurn turn);

// 几张 sprite 按原尺寸拼成一张图：sprite_buf_begin 第一次调用时分配 w×h 的缓冲（规则同上）并清空，
// 再用 sprite_buf_paint_at 把每张叠画到 (x, y)，透明像素不覆盖下面的
lv_draw_buf_t* sprite_buf_begin(lv_draw_buf_t** buf_slot, int32_t w, int32_t h);
void sprite_buf_paint_at(lv_draw_buf_t* buf, const Sprite* sprite, int32_t x, int32_t y);

#endif
