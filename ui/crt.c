/**
 * CRT 显示效果
 */

#include "crt.h"

#define BEZEL_W   6   // 机壳宽度
#define OPEN_R    12  // 屏幕开口的圆角半径
#define CORNER    (BEZEL_W + OPEN_R)
#define SCANLINE_KEEP 218  // 扫描线保留的亮度，/256，即压暗 15%

#define COLOR_EDGE  0x18171D  // 机壳最外圈
#define COLOR_BEZEL 0x45404F
#define COLOR_LIP   0x111116  // 机壳贴着屏幕的一圈暗边
#define COLOR_GLOW  0xC9F25B  // 屏幕内侧发光线

enum { CLS_CONTENT, CLS_GLOW_WEAK, CLS_GLOW, CLS_LIP, CLS_BEZEL, CLS_EDGE };

static uint8_t s_corner[CORNER][CORNER];  // [离上下边距离][离左右边距离]，两者都小于 CORNER 时用
static uint8_t s_side[CORNER];            // 只有一边离得近时，按离边距离查
static int32_t s_w;
static int32_t s_h;
static uint16_t s_edge, s_bezel, s_lip, s_glow_half, s_glow_quarter;
static bool s_scanlines = true;
static bool s_ready;

static uint16_t to_rgb565(uint32_t hex) {
    return lv_color_to_u16(lv_color_hex(hex));
}

static uint16_t half(uint16_t c) {
    return (c >> 1) & 0x7BEF;
}

static uint16_t quarter(uint16_t c) {
    return (c >> 2) & 0x39E7;
}

// 按像素中心到开口边缘的深度分类，深度 = OPEN_R - 到圆心的距离；都用两倍坐标的平方比较，避开开方
static uint8_t classify(int32_t dist2x4, int32_t edge_dist) {
    if (edge_dist == 0) return CLS_EDGE;
    if (dist2x4 > (2 * OPEN_R + 2) * (2 * OPEN_R + 2)) return CLS_BEZEL;
    if (dist2x4 > (2 * OPEN_R) * (2 * OPEN_R)) return CLS_LIP;
    if (dist2x4 > (2 * OPEN_R - 2) * (2 * OPEN_R - 2)) return CLS_GLOW;
    if (dist2x4 > (2 * OPEN_R - 4) * (2 * OPEN_R - 4)) return CLS_GLOW_WEAK;
    return CLS_CONTENT;
}

static void build_tables(void) {
    for (int32_t y = 0; y < CORNER; y++) {
        for (int32_t x = 0; x < CORNER; x++) {
            int32_t cx = 2 * CORNER - 2 * x - 1;
            int32_t cy = 2 * CORNER - 2 * y - 1;
            s_corner[y][x] = classify(cx * cx + cy * cy, x < y ? x : y);
        }
    }
    // 直边：离边 i 的像素等同于圆心正下方同样深度的像素
    for (int32_t i = 0; i < CORNER; i++) {
        int32_t c = 2 * CORNER - 2 * i - 1;
        s_side[i] = classify(c * c, i);
    }
    s_edge = to_rgb565(COLOR_EDGE);
    s_bezel = to_rgb565(COLOR_BEZEL);
    s_lip = to_rgb565(COLOR_LIP);
    s_glow_half = half(to_rgb565(COLOR_GLOW));
    s_glow_quarter = quarter(to_rgb565(COLOR_GLOW));
}

static uint16_t dim(uint16_t c) {
    uint32_t r = ((c >> 11) * SCANLINE_KEEP + 128) >> 8;
    uint32_t g = (((c >> 5) & 0x3F) * SCANLINE_KEEP + 128) >> 8;
    uint32_t b = ((c & 0x1F) * SCANLINE_KEEP + 128) >> 8;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static uint16_t shade(uint16_t c, uint8_t cls, bool scanline) {
    switch (cls) {
        case CLS_EDGE:  return s_edge;
        case CLS_BEZEL: return s_bezel;
        case CLS_LIP:   return s_lip;
        case CLS_GLOW:  c = half(c) + s_glow_half; break;
        case CLS_GLOW_WEAK: c = c - quarter(c) + s_glow_quarter; break;
        default: break;
    }
    return scanline ? dim(c) : c;
}

void crt_apply(uint16_t* px, int32_t stride_px, const lv_area_t* area) {
    if (!s_ready) return;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        uint16_t* row = px + (y - area->y1) * stride_px - area->x1;
        int32_t dy = y < s_h - 1 - y ? y : s_h - 1 - y;
        bool scanline = s_scanlines && (y & 1);
        for (int32_t x = area->x1; x <= area->x2; x++) {
            int32_t dx = x < s_w - 1 - x ? x : s_w - 1 - x;
            uint8_t cls;
            if (dx >= CORNER && dy >= CORNER) {
                if (!scanline) {
                    x = s_w - 1 - CORNER;  // 中间没有要改的，跳到右边机壳
                    continue;
                }
                cls = CLS_CONTENT;
            } else if (dx < CORNER && dy < CORNER) {
                cls = s_corner[dy][dx];
            } else {
                cls = s_side[dx < dy ? dx : dy];
            }
            row[x] = shade(row[x], cls, scanline);
        }
    }
}

static void on_flush_start(lv_event_t* e) {
    lv_display_t* disp = lv_event_get_target(e);
    const lv_area_t* area = lv_event_get_param(e);
    lv_draw_buf_t* buf = lv_display_get_buf_active(disp);
    if (buf == NULL || buf->header.cf != LV_COLOR_FORMAT_RGB565) return;
    int32_t stride_px = (int32_t)(buf->header.stride / 2);
    uint16_t* px = (uint16_t*)buf->data;
    // 局部刷新时缓冲区只有这块区域；整屏/直接模式时缓冲区是整屏
    if (lv_display_get_render_mode(disp) != LV_DISPLAY_RENDER_MODE_PARTIAL) {
        px += area->y1 * stride_px + area->x1;
    }
    crt_apply(px, stride_px, area);
}

void crt_init(lv_display_t* disp) {
    s_w = lv_display_get_horizontal_resolution(disp);
    s_h = lv_display_get_vertical_resolution(disp);
    build_tables();
    s_ready = true;
    lv_display_add_event_cb(disp, on_flush_start, LV_EVENT_FLUSH_START, NULL);
}

void crt_set_scanlines(bool on) {
    s_scanlines = on;
    lv_obj_invalidate(lv_screen_active());
}

bool crt_scanlines_enabled(void) {
    return s_scanlines;
}
