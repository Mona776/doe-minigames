/**
 * 像素画精灵
 */

#include "sprite.h"
#include <string.h>
#include "ui/ui_theme.h"

static uint32_t palette(char c) {
    switch (c) {
        case 'K': return UI_COLOR_INK;     // 描边
        case 'W': return UI_COLOR_CREAM;   // 身体
        case 'B': return 0xC08552;         // 棕色斑
        case 'E': return 0x8A5530;         // 深棕：耳朵、尾巴、远侧的腿
        case 'P': return 0xE88A9A;         // 鼻子
        case 'R': return UI_COLOR_ALERT;   // 红
        case 'Y': return UI_COLOR_FOCUS;   // 黄
        case 'G': return 0x8A8F9C;         // 石头
        case 'D': return 0x5A5E6B;         // 石头暗面
        case 'L': return 0xD4F04C;         // 网球
        default:  return 0xFF00FF;         // 调色板里没有的字母，显眼的品红
    }
}

lv_obj_t* sprite_canvas_create(lv_obj_t* parent, lv_draw_buf_t** buf_slot, int32_t w, int32_t h) {
    if (*buf_slot == NULL) *buf_slot = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_ARGB8888, 0);
    lv_obj_t* canvas = lv_canvas_create(parent);
    lv_canvas_set_draw_buf(canvas, *buf_slot);
    return canvas;
}

static void paint(lv_draw_buf_t* buf, const Sprite* sprite, int32_t scale, SpriteTurn turn) {
    lv_draw_buf_clear(buf, NULL);
    int32_t bw = buf->header.w;
    int32_t bh = buf->header.h;
    int32_t n = sprite->h;
    int32_t w = turn.rot % 2 ? n : (int32_t)strlen(sprite->rows[0]);
    for (int32_t y = 0; y < sprite->h; y++) {
        const char* row = sprite->rows[y];
        for (int32_t x = 0; row[x]; x++) {
            if (row[x] == '.') continue;
            uint32_t argb = 0xFF000000u | palette(row[x]);
            int32_t dx = x, dy = y;
            for (uint8_t r = 0; r < turn.rot % 4; r++) {
                int32_t t = dx;
                dx = n - 1 - dy;
                dy = t;
            }
            if (turn.mirror) dx = w - 1 - dx;
            for (int32_t py = dy * scale; py < (dy + 1) * scale && py < bh; py++) {
                uint32_t* line = (uint32_t*)(buf->data + py * buf->header.stride);
                for (int32_t px = dx * scale; px < (dx + 1) * scale && px < bw; px++) line[px] = argb;
            }
        }
    }
}

void sprite_draw(lv_obj_t* canvas, const Sprite* sprite, int32_t scale) {
    sprite_draw_turned(canvas, sprite, scale, (SpriteTurn){0, false});
}

void sprite_draw_turned(lv_obj_t* canvas, const Sprite* sprite, int32_t scale, SpriteTurn turn) {
    paint(lv_canvas_get_draw_buf(canvas), sprite, scale, turn);
    lv_obj_invalidate(canvas);
}

lv_draw_buf_t* sprite_buf_render(lv_draw_buf_t** buf_slot, const Sprite* sprite, int32_t scale, SpriteTurn turn) {
    int32_t size = sprite->h * scale;
    if (*buf_slot == NULL) *buf_slot = lv_draw_buf_create(size, size, LV_COLOR_FORMAT_ARGB8888, 0);
    paint(*buf_slot, sprite, scale, turn);
    return *buf_slot;
}

lv_draw_buf_t* sprite_buf_begin(lv_draw_buf_t** buf_slot, int32_t w, int32_t h) {
    if (*buf_slot == NULL) *buf_slot = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_ARGB8888, 0);
    lv_draw_buf_clear(*buf_slot, NULL);
    return *buf_slot;
}

void sprite_buf_paint_at(lv_draw_buf_t* buf, const Sprite* sprite, int32_t x, int32_t y) {
    for (int32_t sy = 0; sy < sprite->h; sy++) {
        int32_t py = y + sy;
        if (py < 0 || py >= (int32_t)buf->header.h) continue;
        uint32_t* line = (uint32_t*)(buf->data + py * buf->header.stride);
        const char* row = sprite->rows[sy];
        for (int32_t sx = 0; row[sx]; sx++) {
            int32_t px = x + sx;
            if (row[sx] == '.' || px < 0 || px >= (int32_t)buf->header.w) continue;
            line[px] = 0xFF000000u | palette(row[sx]);
        }
    }
}
