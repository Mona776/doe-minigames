/**
 * 贪吃腊肠 - 三关，每关开始倒计时 3、2、1、GO，之后 Doe 自动前进；从入口页（page_snake_entry.c）替换过来
 * 按键：B（确定）顺时针转 90°（按下即生效，按住不重复），A（选择）暂停/继续，
 * C（返回）弹出退出确认（倒计时、走动、换关时；弹窗打开时游戏冻结），暂停中按 C 直接回小游戏菜单、没有奖励
 * 规则在 core/games/snake.c；这里每帧推进规则，10×10 网格每格一个图片对象，
 * 侧面看的腊肠由头、身体、转角、尾巴拼成（向左走时镜像，竖着走时转 90°），
 * 前脚挂在头下、后脚挂在尾巴下，走动时两对脚交替迈步；头前面的小箭头是下一步的方向
 * 每关的石头由规则随机摆好，这里按 blocked 画出来；倒计时数字压在场地上方，数到 1 时变淡
 * 过关时先显示「本关完成」再显示下一关的 ROUND；撞到后腊肠闪几下再显示结算，并把奖励作为命令发给逻辑服务；
 * 结算页 B 再来一局，C 回入口页
 * 音效是复古街机风格（滑音、快速琶音、噪声），走路和转向不出声：吃到骨头一声上扬的「啾」，
 * 倒计时「嘀、嘀、嘀、嘀——」，过关一串快速上行琶音，通关是小号式的短奏加长音，撞到先一下噪声再一路滑下去
 * 特效都按时间戳逐帧算：骨头单独一张图，出现时弹出来、平时上下浮动；吃到时炸出碎屑、分数弹一下；
 * 撞到时整个场地（放在一个容器里）左右震，头上转一圈星星，整条腊肠闪烁，撞到的石头或身体闪红、撞到的墙闪烁
 */

#include <stdio.h>
#include "pages.h"
#include "core/app_msg.h"
#include "core/games/snake.h"
#include "hal/hal_clock.h"
#include "ui/ui_sfx.h"
#include "ui/ui_theme.h"
#include "game_art.h"
#include "snake_art.h"
#include "snake_ui.h"

#define FRAME_MS       15
#define MAX_STEP_MS    100  // 卡顿时一帧最多推进这么久
#define RESULT_LOCK_MS 800  // 结算出现后这么久内不接受「再来」，防止连按误触
#define CRASH_MS       900  // 撞到后腊肠闪这么久再出结算
#define BLINK_MS       120
#define SCORE_FLASH_MS 400  // 吃到骨头时分数亮一下
#define LEG_FRAMES_PER_STEP 4  // 每走一格脚换几次姿势

#define CELL_PX        20
#define TILE_SCALE     (CELL_PX / SNAKE_TILE)
#define FIELD_PX       (SNAKE_SIZE * CELL_PX)
#define FIELD_X        ((240 - FIELD_PX) / 2)
#define FIELD_Y        30
#define WALL_W         2
#define LEGS_PX        (SNAKE_LEGS * TILE_SCALE)
#define LEGS_INSET     ((LEGS_PX - CELL_PX) / 2)
#define ARROW_SCALE    2
#define ARROW_PX       (SNAKE_ARROW * ARROW_SCALE)
#define ARROW_OFFSET   14   // 箭头中心离头中心的距离，刚好露在头前面
#define DIGIT_SCALE    4
#define DIGIT_Y        (FIELD_Y + 4)  // 倒计时数字压在场地最上面两行，离出发那一行远
#define DIGIT_LAST_OPA LV_OPA_40      // 数到 1 时变淡，让玩家看清整个场地

#define BONE_POP_MS    250  // 新骨头从小弹到稍大再落回原大
#define BONE_BOB_MS    600  // 骨头每隔这么久上下换一次
#define BONE_BOB_PX    TILE_SCALE
#define SCORE_POP_MS   250
#define SCORE_POP_PEAK 80   // 分数弹起时多放大多少（256 = 1 倍）
#define DEBRIS_N       4
#define DEBRIS_MS      320
#define DEBRIS_PX      (2 * TILE_SCALE)
#define DEBRIS_G       700  // 碎屑下落的加速度，像素/秒²
#define SHAKE_MS       320
#define SHAKE_PX       4
#define SHAKE_HALF_MS  35   // 每隔这么久换一次方向
#define STAR_N         3
#define STAR_SCALE     2
#define STAR_PX        (SNAKE_STAR * STAR_SCALE)
#define STAR_TURN_MS   600  // 星星转一圈的时间
#define STAR_RX        11
#define STAR_RY        4
#define STAR_LIFT      13   // 星星转的圈离头中心多高

// 碎屑的初速度，像素/秒
static const int16_t DEBRIS_V[DEBRIS_N][2] = {
    {-90, -130}, {-30, -170}, {40, -160}, {100, -110},
};

static const UiNote SFX_EAT[] = {{1100, 22, 1700}};
static const UiNote SFX_COUNT[] = {{440, 90}};
static const UiNote SFX_GO[] = {{880, 70}, {0, 20}, {1760, 240}};
static const UiNote SFX_LEVEL_UP[] = {
    {523, 35}, {659, 35}, {784, 35}, {1047, 35},
    {659, 35}, {784, 35}, {1047, 35}, {1319, 35},
    {784, 35}, {1047, 35}, {1319, 35}, {1568, 35},
    {2093, 70}, {2217, 50}, {2093, 160},
};
static const UiNote SFX_CLEAR[] = {
    {784, 90}, {0, 25}, {784, 90}, {0, 25}, {784, 90}, {0, 25}, {1047, 260},
    {932, 260}, {1175, 260}, {1047, 90}, {0, 25}, {1175, 90}, {1568, 500, 1600},
};
static const UiNote SFX_CRASH[] = {{90, 140, 480, true}, {700, 420, 70}};
static const UiNote SFX_PAUSE[] = {{1319, 35}, {0, 25}, {988, 45}};

#define COLOR_CELL_A   UI_COLOR_PANEL
#define COLOR_CELL_B   0x262935

// 图是朝右、肚子朝下画的：往左走镜像，竖着走转 90°，肚子和脚始终在下方或右侧
static const SpriteTurn TURN[] = {
    [SNAKE_UP]    = {3, false},
    [SNAKE_RIGHT] = {0, false},
    [SNAKE_DOWN]  = {1, true},
    [SNAKE_LEFT]  = {0, true},
};

#define ICON_SCALE       2
#define ICON_PX          (SNAKE_ICON * ICON_SCALE)
#define RESULT_STAR_SCALE 3
#define DIM_OPA          LV_OPA_60  // 暂停、退出确认时压暗场景

enum { CONFIRM_STAY, CONFIRM_EXIT, CONFIRM_COUNT };

static SnakeGame s_game;
static lv_timer_t* s_timer;
static uint32_t s_last_tick;
static uint32_t s_over_at;
static uint32_t s_result_at;
static uint32_t s_score_at;
static uint32_t s_bone_at;
static uint32_t s_eat_at;
static uint8_t s_eat_cell;
static bool s_result_shown;
static bool s_confirm_open;
static uint8_t s_confirm_focus;

// 界面上次画的状态，变了才重画
static uint16_t s_shown_moves;
static uint8_t s_shown_bone;
static SnakePhase s_shown_phase;
static uint8_t s_shown_level_bones;
static uint8_t s_shown_level;
static uint8_t s_shown_count;
static int8_t s_shown_level_up;  // 换关提示显示的是第几段：0 本关完成，1 下一关
static const void* s_cell_src[SNAKE_CELLS];
static int32_t s_score_scale;

typedef struct {
    lv_obj_t* canvas;
    const Sprite* frames;
    int8_t frame;
    int8_t dir;
} Legs;

// 拼接用的小图，各个方向各一份，所有格子共用
static lv_draw_buf_t* s_head_buf[4];
static lv_draw_buf_t* s_tail_buf[4];
static lv_draw_buf_t* s_body_buf[2];  // 横、竖
static lv_draw_buf_t* s_corner_buf[4];
static lv_draw_buf_t* s_bone_buf;
static lv_draw_buf_t* s_rock_buf;
static lv_draw_buf_t* s_arrow_buf[4];
static lv_draw_buf_t* s_front_buf;
static lv_draw_buf_t* s_back_buf;
static lv_draw_buf_t* s_digit_buf;
static lv_draw_buf_t* s_star_buf;
static lv_draw_buf_t* s_icon_bone_buf;
static lv_draw_buf_t* s_icon_shell_buf;
static lv_draw_buf_t* s_icon_star_buf[2];

static lv_obj_t* s_field;  // 墙、格子、脚、箭头和特效都在这里面，震动时只挪它
static lv_obj_t* s_wall;
static lv_obj_t* s_cells[SNAKE_CELLS];
static lv_obj_t* s_bone;
static lv_obj_t* s_hit;    // 撞到的那格上闪的红块
static lv_obj_t* s_debris[DEBRIS_N];
static lv_obj_t* s_stars[STAR_N];
static Legs s_front;
static Legs s_back;
static lv_obj_t* s_arrow;
static lv_obj_t* s_level;
static lv_obj_t* s_score_box;  // 骨头图标 + 本关进度，吃到时整体弹一下
static lv_obj_t* s_score;
static lv_obj_t* s_digit;
static lv_obj_t* s_go;
static lv_obj_t* s_level_up;
static lv_obj_t* s_level_up_title;
static lv_obj_t* s_pause;
static lv_obj_t* s_confirm;
static lv_obj_t* s_confirm_options[CONFIRM_COUNT];
static lv_obj_t* s_result;
static lv_obj_t* s_result_title;
static lv_obj_t* s_result_bones;
static lv_obj_t* s_result_stars[3];
static lv_obj_t* s_result_shells;

static lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static lv_obj_t* column_panel(lv_obj_t* scr, int32_t w, int32_t h) {
    lv_obj_t* p = ui_panel_create(scr);
    lv_obj_set_size(p, w, h);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 4, 0);
    return p;
}

static SnakeDir dir_between(uint8_t from, uint8_t to) {
    int d = (int)to - (int)from;
    if (d == 1) return SNAKE_RIGHT;
    if (d == -1) return SNAKE_LEFT;
    return d > 0 ? SNAKE_DOWN : SNAKE_UP;
}

// 尾巴那一节朝哪走（朝它前面一节）
static SnakeDir tail_dir(const SnakeGame* g) {
    return dir_between(snake_segment(g, g->len - 1), snake_segment(g, g->len - 2));
}

static const void* segment_tile(const SnakeGame* g, uint8_t i) {
    uint8_t c = snake_segment(g, i);
    if (i == 0) return s_head_buf[g->dir];
    if (i == g->len - 1) return s_tail_buf[tail_dir(g)];
    SnakeDir to_head = dir_between(c, snake_segment(g, i - 1));
    SnakeDir to_tail = dir_between(c, snake_segment(g, i + 1));
    if ((to_head + 2) % 4 == to_tail) return s_body_buf[to_head % 2 ? 0 : 1];
    // 转角图接的是下、左两边，即相邻两个方向里靠前的是「下」，顺时针转到实际的那一对
    SnakeDir first = (to_tail - to_head + 4) % 4 == 1 ? to_head : to_tail;
    return s_corner_buf[(first + 2) % 4];
}

static void set_cell(uint8_t c, const void* src) {
    if (s_cell_src[c] == src) return;
    s_cell_src[c] = src;
    lv_image_set_src(s_cells[c], src);
}

static void draw_field(const SnakeGame* g) {
    const void* src[SNAKE_CELLS] = {0};
    for (uint8_t c = 0; c < SNAKE_CELLS; c++) {
        if (g->blocked[c]) src[c] = s_rock_buf;
    }
    for (uint8_t i = 0; i < g->len; i++) src[snake_segment(g, i)] = segment_tile(g, i);
    for (uint8_t c = 0; c < SNAKE_CELLS; c++) set_cell(c, src[c]);
}

static void cell_origin(uint8_t c, int32_t* x, int32_t* y) {
    *x = FIELD_X + (c % SNAKE_SIZE) * CELL_PX;
    *y = FIELD_Y + (c / SNAKE_SIZE) * CELL_PX;
}

static void place_legs(Legs* legs, uint8_t c, SnakeDir dir, int8_t frame) {
    if (legs->frame != frame || legs->dir != (int8_t)dir) {
        legs->frame = frame;
        legs->dir = (int8_t)dir;
        sprite_draw_turned(legs->canvas, &legs->frames[frame], TILE_SCALE, TURN[dir]);
    }
    int32_t x, y;
    cell_origin(c, &x, &y);
    lv_obj_set_pos(legs->canvas, x - LEGS_INSET, y - LEGS_INSET);
}

// 走动时按节拍内的进度换姿势，前后两对脚错开；其他时候站着
static void render_legs(const SnakeGame* g) {
    int8_t frame = 0;
    if (g->phase == SNAKE_PHASE_RUN) {
        frame = (int8_t)((g->phase_ms * LEG_FRAMES_PER_STEP / snake_step_ms(g->level)) % 2);
    }
    place_legs(&s_front, snake_segment(g, 0), g->dir, frame);
    place_legs(&s_back, snake_segment(g, g->len - 1), tail_dir(g), g->phase == SNAKE_PHASE_RUN ? 1 - frame : 0);
}

static void place_arrow(const SnakeGame* g) {
    bool shown = g->phase != SNAKE_PHASE_OVER && g->phase != SNAKE_PHASE_LEVEL_UP;
    lv_obj_set_hidden(s_arrow, !shown);
    if (!shown) return;
    SnakeDir dir = snake_next_dir(g);
    int32_t cx, cy;
    cell_origin(snake_segment(g, 0), &cx, &cy);
    cx += CELL_PX / 2;
    cy += CELL_PX / 2;
    if (dir == SNAKE_RIGHT) cx += ARROW_OFFSET;
    if (dir == SNAKE_LEFT) cx -= ARROW_OFFSET;
    if (dir == SNAKE_DOWN) cy += ARROW_OFFSET;
    if (dir == SNAKE_UP) cy -= ARROW_OFFSET;
    const void* src = s_arrow_buf[dir];
    if (lv_image_get_src(s_arrow) != src) lv_image_set_src(s_arrow, src);
    lv_obj_set_pos(s_arrow, cx - ARROW_PX / 2, cy - ARROW_PX / 2);
}

static void show(lv_obj_t* obj, bool shown) {
    if (lv_obj_is_hidden(obj) == shown) lv_obj_set_hidden(obj, !shown);
}

static void cell_center(uint8_t c, int32_t* x, int32_t* y) {
    cell_origin(c, x, y);
    *x += CELL_PX / 2;
    *y += CELL_PX / 2;
}

static uint8_t cell_ahead(uint8_t c, SnakeDir dir) {
    switch (dir) {
        case SNAKE_UP:   return (uint8_t)(c - SNAKE_SIZE);
        case SNAKE_DOWN: return (uint8_t)(c + SNAKE_SIZE);
        case SNAKE_LEFT: return (uint8_t)(c - 1);
        default:         return (uint8_t)(c + 1);
    }
}

// 后 1/2 时间里从不透明淡到全透明
static lv_opa_t fade_out(uint32_t t, uint32_t total) {
    if (t < total / 2) return LV_OPA_COVER;
    return (lv_opa_t)(LV_OPA_COVER * (total - t) / (total - total / 2));
}

// 弹出的缩放：前 3/5 时间从 from 涨到 256 + peak，剩下的时间落回 256
static int32_t pop_scale(uint32_t t, uint32_t total, int32_t from, int32_t peak) {
    uint32_t rise = total * 3 / 5;
    if (t >= total) return 256;
    if (t < rise) return from + (256 + peak - from) * (int32_t)t / (int32_t)rise;
    return 256 + peak - peak * (int32_t)(t - rise) / (int32_t)(total - rise);
}

// 骨头被身体压住（刚吃掉、过关时）就不显示
static void render_bone(const SnakeGame* g, uint32_t now) {
    bool shown = !g->occupied[g->bone];
    show(s_bone, shown);
    if (!shown) return;
    int32_t scale = pop_scale(lv_tick_diff(now, s_bone_at), BONE_POP_MS, 16, 64);
    lv_image_set_scale(s_bone, (uint32_t)scale);
    int32_t x, y;
    cell_origin(g->bone, &x, &y);
    int32_t bob = (now / BONE_BOB_MS) % 2 ? BONE_BOB_PX : 0;
    lv_obj_set_pos(s_bone, x, y - bob);
}

// 吃到骨头：碎屑带重力往上方四散、慢慢淡掉，分数弹一下
static void render_eat(uint32_t now) {
    uint32_t t = lv_tick_diff(now, s_eat_at);
    int32_t ox, oy;
    cell_center(s_eat_cell, &ox, &oy);
    for (uint8_t i = 0; i < DEBRIS_N; i++) {
        bool shown = t < DEBRIS_MS;
        show(s_debris[i], shown);
        if (!shown) continue;
        int32_t ms = (int32_t)t;
        int32_t x = ox + DEBRIS_V[i][0] * ms / 1000;
        int32_t y = oy + DEBRIS_V[i][1] * ms / 1000 + DEBRIS_G * ms * ms / 2000000;
        // 对齐到像素画的格子上，飞起来也是一格一格的
        x = x / TILE_SCALE * TILE_SCALE;
        y = y / TILE_SCALE * TILE_SCALE;
        lv_obj_set_pos(s_debris[i], x - DEBRIS_PX / 2, y - DEBRIS_PX / 2);
        lv_obj_set_style_opa(s_debris[i], fade_out(t, DEBRIS_MS), 0);
    }
    int32_t scale = pop_scale(lv_tick_diff(now, s_score_at), SCORE_POP_MS, 256, SCORE_POP_PEAK);
    if (scale != s_score_scale) {
        s_score_scale = scale;
        lv_obj_set_style_transform_scale(s_score_box, scale, 0);
    }
}

static void show_snake(const SnakeGame* g, bool shown) {
    for (uint8_t i = 0; i < g->len; i++) set_cell(snake_segment(g, i), shown ? segment_tile(g, i) : NULL);
    show(s_front.canvas, shown);
    show(s_back.canvas, shown);
}

// 撞到：场地左右震、越震越轻；头上转星星；整条腊肠一闪一闪，撞到的石头或身体跟着闪红；
// 撞墙时墙也跟着闪，结算出来后还接着闪
static void render_crash(const SnakeGame* g, uint32_t now) {
    bool crashed = g->phase == SNAKE_PHASE_OVER && g->end != SNAKE_END_CLEAR && !s_result_shown;
    uint32_t t = crashed ? lv_tick_diff(now, s_over_at) : 0;

    int32_t dx = 0;
    if (crashed && t < SHAKE_MS) {
        int32_t amp = (SHAKE_PX * (int32_t)(SHAKE_MS - t) + SHAKE_MS - 1) / SHAKE_MS;
        dx = (t / SHAKE_HALF_MS) % 2 ? -amp : amp;
    }
    if (lv_obj_get_x(s_field) != dx) lv_obj_set_x(s_field, dx);

    bool on = crashed && (t / BLINK_MS) % 2 == 0;
    if (crashed) show_snake(g, on);
    bool wall_crash = g->phase == SNAKE_PHASE_OVER && g->end == SNAKE_END_WALL;
    show(s_wall, !wall_crash || (lv_tick_diff(now, s_over_at) / BLINK_MS) % 2 == 0);
    bool hit = on && g->end != SNAKE_END_WALL;
    show(s_hit, hit);
    uint8_t head = snake_segment(g, 0);
    if (hit) {
        int32_t x, y;
        cell_origin(cell_ahead(head, g->dir), &x, &y);
        lv_obj_set_pos(s_hit, x, y);
    }

    int32_t hx, hy;
    cell_center(head, &hx, &hy);
    for (uint8_t i = 0; i < STAR_N; i++) {
        show(s_stars[i], crashed);
        if (!crashed) continue;
        int16_t angle = (int16_t)((t * 360 / STAR_TURN_MS + i * 360 / STAR_N) % 360);
        int32_t x = hx + STAR_RX * lv_trigo_cos(angle) / LV_TRIGO_SIN_MAX;
        int32_t y = hy - STAR_LIFT + STAR_RY * lv_trigo_sin(angle) / LV_TRIGO_SIN_MAX;
        lv_obj_set_pos(s_stars[i], x - STAR_PX / 2, y - STAR_PX / 2);
    }
}

static void render_countdown(const SnakeGame* g) {
    uint8_t n = g->phase == SNAKE_PHASE_COUNTDOWN ? snake_countdown(g) : 0xFF;
    if (n == s_shown_count) return;
    s_shown_count = n;
    lv_obj_set_hidden(s_digit, n == 0 || n == 0xFF);
    lv_obj_set_hidden(s_go, n != 0);
    if (n && n != 0xFF) sprite_draw(s_digit, &game_digits[n - 1], DIGIT_SCALE);
    lv_obj_set_style_opa(s_digit, n == 1 ? DIGIT_LAST_OPA : LV_OPA_COVER, 0);
    if (n == 0) UI_SFX_PLAY(SFX_GO);
    if (n && n != 0xFF) UI_SFX_PLAY(SFX_COUNT);
}

static void render_level(const SnakeGame* g) {
    if (g->level == s_shown_level) return;
    s_shown_level = g->level;
    lv_label_set_text_fmt(s_level, "ROUND %u", (unsigned)g->level);
}

// 换关停顿的前一半显示「本关完成」，后一半显示下一关
static void render_level_up(const SnakeGame* g) {
    if (g->phase != SNAKE_PHASE_LEVEL_UP) return;
    int8_t part = g->phase_ms < SNAKE_LEVEL_UP_MS / 2 ? 0 : 1;
    if (part == s_shown_level_up) return;
    s_shown_level_up = part;
    if (part == 0) {
        lv_label_set_text(s_level_up_title, "本关完成");
    } else {
        lv_label_set_text_fmt(s_level_up_title, "ROUND %u", (unsigned)g->level + 1);
    }
    lv_obj_set_style_text_color(s_level_up_title, lv_color_hex(part == 0 ? UI_COLOR_GLOW : UI_COLOR_FOCUS), 0);
}

static void enter_phase(const SnakeGame* g, uint32_t now) {
    lv_obj_set_hidden(s_pause, g->phase != SNAKE_PHASE_PAUSED);
    lv_obj_set_hidden(s_level_up, g->phase != SNAKE_PHASE_LEVEL_UP);
    s_shown_level_up = -1;
    if (g->phase == SNAKE_PHASE_OVER) s_over_at = now;
}

// 同一帧里有几件事时只响最重要的：结束、过关、吃到骨头
static void play_sfx(const SnakeGame* g, bool entered, bool ate) {
    if (entered && g->phase == SNAKE_PHASE_OVER) {
        if (g->end == SNAKE_END_CLEAR) {
            UI_SFX_PLAY(SFX_CLEAR);
        } else {
            UI_SFX_PLAY(SFX_CRASH);
        }
    } else if (entered && g->phase == SNAKE_PHASE_LEVEL_UP) {
        UI_SFX_PLAY(SFX_LEVEL_UP);
    } else if (ate) {
        UI_SFX_PLAY(SFX_EAT);
    }
}

static void render(void) {
    const SnakeGame* g = &s_game;
    uint32_t now = lv_tick_get();
    render_level(g);
    render_countdown(g);

    bool ate = g->level_bones > s_shown_level_bones && s_shown_level_bones != 0xFF;
    bool phase_changed = g->phase != s_shown_phase;
    if (phase_changed) {
        s_shown_phase = g->phase;
        enter_phase(g, now);
    }
    render_level_up(g);
    if (g->bone != s_shown_bone) s_bone_at = now;
    if (phase_changed || g->moves != s_shown_moves || g->bone != s_shown_bone) {
        s_shown_moves = g->moves;
        s_shown_bone = g->bone;
        draw_field(g);
    }
    render_bone(g, now);
    render_legs(g);
    place_arrow(g);

    if (ate) {
        s_eat_at = now;
        s_eat_cell = snake_segment(g, 0);
    }
    render_eat(now);
    render_crash(g, now);

    if (g->level_bones != s_shown_level_bones) {
        if (g->level_bones > s_shown_level_bones) s_score_at = now;
        s_shown_level_bones = g->level_bones;
        lv_label_set_text_fmt(s_score, "%u/%u", (unsigned)g->level_bones, (unsigned)SNAKE_LEVEL_BONES);
    }
    bool flash = lv_tick_diff(now, s_score_at) < SCORE_FLASH_MS;
    lv_obj_set_style_text_color(s_score, lv_color_hex(flash ? UI_COLOR_GLOW : UI_COLOR_TEXT), 0);
    play_sfx(g, phase_changed, ate);
}

static const char* end_title(SnakeEnd end) {
    switch (end) {
        case SNAKE_END_WALL:     return "撞墙了";
        case SNAKE_END_OBSTACLE: return "撞到石头了";
        case SNAKE_END_SELF:     return "咬到自己了";
        case SNAKE_END_CLEAR:    return "通关!";
        default:                 return "";
    }
}

static void show_result(void) {
    const SnakeGame* g = &s_game;
    SnakeReward reward = snake_reward(g->bones);
    AppCmd cmd = {
        .type = APP_CMD_GAME_RESULT,
        .result = {.game = GAME_SNAKE, .stars = reward.stars, .shells = reward.shells, .spirit = reward.spirit},
    };
    app_cmd_send(&cmd);

    bool cleared = g->end == SNAKE_END_CLEAR;
    lv_label_set_text(s_result_title, cleared ? "全部通关" : "本局结束");
    lv_obj_set_style_text_color(s_result_title, lv_color_hex(cleared ? UI_COLOR_GLOW : UI_COLOR_FOCUS), 0);
    lv_label_set_text_fmt(s_result_bones, "%u", (unsigned)g->bones);
    for (uint8_t i = 0; i < 3; i++) lv_image_set_src(s_result_stars[i], s_icon_star_buf[i < reward.stars]);
    lv_label_set_text_fmt(s_result_shells, "+%u", (unsigned)reward.shells);
    lv_obj_set_hidden(s_result, false);
    show_snake(g, true);

    s_result_shown = true;
    s_result_at = lv_tick_get();
    printf("[Snake] %s，第 %u 关，吃到 %u 根，%u 星，用时 %.1f 秒\n", end_title(g->end), (unsigned)g->level,
           (unsigned)g->bones, (unsigned)reward.stars, g->elapsed_ms / 1000.0);
}

static void advance(void) {
    uint32_t now = lv_tick_get();
    uint32_t dt = now - s_last_tick;
    s_last_tick = now;
    snake_step(&s_game, dt > MAX_STEP_MS ? MAX_STEP_MS : dt);
}

static void on_frame(lv_timer_t* timer) {
    (void)timer;
    if (!s_confirm_open) advance();
    render();
    if (s_game.phase == SNAKE_PHASE_OVER && !s_result_shown && !s_confirm_open) {
        uint32_t wait = s_game.end == SNAKE_END_CLEAR ? 0 : CRASH_MS;
        if (lv_tick_elaps(s_over_at) >= wait) show_result();
    }
}

static void start(void) {
    snake_init(&s_game, (uint32_t)hal_clock_us());
    s_last_tick = lv_tick_get();
    s_result_shown = false;
    s_shown_moves = 0xFFFF;
    s_shown_phase = SNAKE_PHASE_OVER;
    s_shown_level_bones = 0xFF;
    s_score_at = s_last_tick - SCORE_FLASH_MS;
    s_eat_at = s_last_tick - DEBRIS_MS;
    s_shown_bone = 0xFF;
    s_shown_level = 0;
    s_shown_count = 0xFE;
    s_confirm_open = false;
    lv_obj_set_hidden(s_confirm, true);
    lv_obj_set_hidden(s_result, true);
    render();
}

static void create_tiles(void) {
    for (uint8_t r = 0; r < 4; r++) {
        sprite_buf_render(&s_head_buf[r], &snake_head, TILE_SCALE, TURN[r]);
        sprite_buf_render(&s_tail_buf[r], &snake_tail, TILE_SCALE, TURN[r]);
        sprite_buf_render(&s_corner_buf[r], &snake_corner, TILE_SCALE, (SpriteTurn){r, false});
        sprite_buf_render(&s_arrow_buf[r], &snake_arrow, ARROW_SCALE, (SpriteTurn){(uint8_t)((r + 3) % 4), false});
    }
    sprite_buf_render(&s_body_buf[0], &snake_body, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_body_buf[1], &snake_body, TILE_SCALE, (SpriteTurn){1, false});
    sprite_buf_render(&s_bone_buf, &snake_bone, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_rock_buf, &snake_rock, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_star_buf, &snake_star, STAR_SCALE, (SpriteTurn){0, false});
    for (uint8_t i = 0; i < 2; i++) {
        sprite_buf_render(&s_icon_star_buf[i], &snake_icon_star[i], 1, (SpriteTurn){0, false});
    }
}

static void create_legs(lv_obj_t* scr, Legs* legs, lv_draw_buf_t** buf, const Sprite* frames) {
    legs->canvas = sprite_canvas_create(scr, buf, LEGS_PX, LEGS_PX);
    legs->frames = frames;
    legs->frame = -1;
    legs->dir = -1;
}

static lv_obj_t* pixel_box(lv_obj_t* parent, int32_t size, uint32_t color, lv_opa_t opa) {
    lv_obj_t* box = ui_box_create(parent);
    lv_obj_set_size(box, size, size);
    lv_obj_set_style_bg_color(box, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(box, opa, 0);
    lv_obj_set_hidden(box, true);
    return box;
}

static void create_field(lv_obj_t* scr) {
    s_field = ui_box_create(scr);
    lv_obj_set_size(s_field, LV_PCT(100), LV_PCT(100));
    lv_obj_t* field = s_field;

    s_wall = ui_box_create(field);
    lv_obj_set_pos(s_wall, FIELD_X - WALL_W, FIELD_Y - WALL_W);
    lv_obj_set_size(s_wall, FIELD_PX + 2 * WALL_W, FIELD_PX + 2 * WALL_W);
    lv_obj_set_style_border_width(s_wall, WALL_W, 0);
    lv_obj_set_style_border_color(s_wall, lv_color_hex(UI_COLOR_LINE), 0);
    // 棋盘格底色方便看清格子
    for (uint8_t c = 0; c < SNAKE_CELLS; c++) {
        int32_t x, y;
        cell_origin(c, &x, &y);
        lv_obj_t* cell = lv_image_create(field);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, CELL_PX, CELL_PX);
        bool dark = (c % SNAKE_SIZE + c / SNAKE_SIZE) % 2 == 0;
        lv_obj_set_style_bg_color(cell, lv_color_hex(dark ? COLOR_CELL_A : COLOR_CELL_B), 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        s_cells[c] = cell;
        s_cell_src[c] = NULL;
    }
    s_bone = lv_image_create(field);
    lv_obj_set_size(s_bone, CELL_PX, CELL_PX);
    lv_image_set_src(s_bone, s_bone_buf);
    s_hit = pixel_box(field, CELL_PX, UI_COLOR_ALERT, LV_OPA_60);

    create_legs(field, &s_front, &s_front_buf, snake_front_legs);
    create_legs(field, &s_back, &s_back_buf, snake_back_legs);
    s_arrow = lv_image_create(field);
    lv_obj_set_size(s_arrow, ARROW_PX, ARROW_PX);

    for (uint8_t i = 0; i < DEBRIS_N; i++) {
        s_debris[i] = pixel_box(field, DEBRIS_PX, UI_COLOR_CREAM, LV_OPA_COVER);
    }
    for (uint8_t i = 0; i < STAR_N; i++) {
        s_stars[i] = lv_image_create(field);
        lv_obj_set_size(s_stars[i], STAR_PX, STAR_PX);
        lv_image_set_src(s_stars[i], s_star_buf);
        lv_obj_set_hidden(s_stars[i], true);
    }
}

// 一行：图标 + 文字，返回文字
static lv_obj_t* icon_row(lv_obj_t* parent, lv_obj_t** row_out, lv_draw_buf_t** buf, const Sprite* icon,
                          const lv_font_t* font, uint32_t color) {
    lv_obj_t* row = ui_box_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 4, 0);
    snake_icon_create(row, buf, icon, ICON_SCALE);
    if (row_out) *row_out = row;
    return label(row, font, color);
}

static void create_header(lv_obj_t* scr) {
    s_level = label(scr, &font_ui_12, UI_COLOR_FOCUS);
    lv_obj_set_pos(s_level, FIELD_X, 11);
    s_score = icon_row(scr, &s_score_box, &s_icon_bone_buf, &snake_icon_bone, &font_ui_12, UI_COLOR_TEXT);
    lv_obj_align(s_score_box, LV_ALIGN_TOP_RIGHT, -FIELD_X, 11 - (ICON_PX - 12) / 2);
    lv_obj_set_style_transform_pivot_x(s_score_box, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(s_score_box, LV_PCT(50), 0);
    s_score_scale = 256;
}

// 盖住整个屏幕的半透明黑层，压暗后面的场景
static lv_obj_t* dim_layer(lv_obj_t* scr) {
    lv_obj_t* dim = ui_box_create(scr);
    lv_obj_set_size(dim, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(dim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dim, DIM_OPA, 0);
    lv_obj_set_hidden(dim, true);
    return dim;
}

static void create_confirm(lv_obj_t* scr) {
    s_confirm = dim_layer(scr);
    lv_obj_t* panel = column_panel(s_confirm, 180, 140);
    lv_obj_center(panel);
    lv_obj_t* line = label(panel, &font_ui_24, UI_COLOR_TEXT);
    lv_label_set_text(line, "退出本局？");
    line = label(panel, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "退出不获得奖励");
    static const char* const text[CONFIRM_COUNT] = {"继续游戏", "确认退出"};
    for (uint8_t i = 0; i < CONFIRM_COUNT; i++) {
        s_confirm_options[i] = snake_option_create(panel, text[i], &font_ui_12, 132, 24);
    }
    line = label(panel, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "A 切换  B 确定  C 取消");
}

static void create_overlays(lv_obj_t* scr) {
    s_digit = sprite_canvas_create(scr, &s_digit_buf, GAME_DIGIT_W * DIGIT_SCALE, GAME_DIGIT_H * DIGIT_SCALE);
    lv_obj_align(s_digit, LV_ALIGN_TOP_MID, 0, DIGIT_Y);
    s_go = label(scr, &font_ui_24, UI_COLOR_GLOW);
    lv_label_set_text(s_go, "GO");
    lv_obj_align(s_go, LV_ALIGN_TOP_MID, 0, DIGIT_Y + (GAME_DIGIT_H * DIGIT_SCALE - 24) / 2);

    s_level_up = column_panel(scr, 150, 48);
    lv_obj_align(s_level_up, LV_ALIGN_CENTER, 0, 10);
    s_level_up_title = label(s_level_up, &font_ui_24, UI_COLOR_GLOW);
    lv_obj_set_hidden(s_level_up, true);

    s_pause = dim_layer(scr);
    lv_obj_t* line = label(s_pause, &font_ui_24, UI_COLOR_FOCUS);
    lv_label_set_text(line, "暂停");
    lv_obj_align(line, LV_ALIGN_CENTER, 0, -6);
    line = label(s_pause, &font_ui_12, UI_COLOR_TEXT);
    lv_label_set_text(line, "A 继续  C 退出");
    lv_obj_align(line, LV_ALIGN_CENTER, 0, 22);

    s_result = column_panel(scr, 192, 156);
    lv_obj_align(s_result, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_pad_row(s_result, 6, 0);
    s_result_title = label(s_result, &font_ui_24, UI_COLOR_FOCUS);
    s_result_bones = icon_row(s_result, NULL, &s_icon_bone_buf, &snake_icon_bone, &font_ui_24, UI_COLOR_TEXT);
    lv_obj_t* stars = ui_box_create(s_result);
    lv_obj_set_size(stars, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(stars, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(stars, 6, 0);
    for (uint8_t i = 0; i < 3; i++) {
        s_result_stars[i] = snake_icon_create(stars, &s_icon_star_buf[0], &snake_icon_star[0], RESULT_STAR_SCALE);
    }
    s_result_shells = icon_row(s_result, NULL, &s_icon_shell_buf, &snake_icon_shell, &font_ui_24, UI_COLOR_TEXT);
    line = label(s_result, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "B 再来一局  C 返回");

    create_confirm(scr);
}

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    ui_screen_dark(scr);
    create_tiles();
    create_header(scr);
    create_field(scr);
    create_overlays(scr);

    start();
    s_timer = lv_timer_create(on_frame, FRAME_MS, NULL);
}

static void destroy(void) {
    if (s_game.phase != SNAKE_PHASE_OVER) printf("[Snake] 中途退出，没有奖励\n");
    if (s_timer) lv_timer_delete(s_timer);
    s_timer = NULL;
}

// 入口、说明、游戏三页互相替换，栈里只有 小游戏菜单 > 当前页，退一层就回到小游戏菜单
static void exit_to_menu(void) {
    page_pop();
}

static void set_confirm_focus(uint8_t focus) {
    s_confirm_focus = focus;
    for (uint8_t i = 0; i < CONFIRM_COUNT; i++) snake_option_set_focused(s_confirm_options[i], i == focus);
}

// 打开时先把规则推进到现在再冻结；关掉后从关掉的时刻接着算，弹窗开着的时间不算进游戏
static void open_confirm(void) {
    advance();
    // 推进到现在时刚好撞到或通关，就走结算，不再弹「退出不获得奖励」
    if (s_game.phase == SNAKE_PHASE_OVER) {
        render();
        return;
    }
    s_confirm_open = true;
    set_confirm_focus(CONFIRM_STAY);
    lv_obj_set_hidden(s_confirm, false);
}

static void close_confirm(void) {
    s_confirm_open = false;
    s_last_tick = lv_tick_get();
    lv_obj_set_hidden(s_confirm, true);
}

static bool on_confirm_input(const InputEvent* e) {
    if (e->type != INPUT_EVT_SHORT_PRESS) return true;
    if (e->key == INPUT_KEY_SELECT) set_confirm_focus((uint8_t)((s_confirm_focus + 1) % CONFIRM_COUNT));
    if (e->key == INPUT_KEY_BACK) close_confirm();
    if (e->key == INPUT_KEY_OK) {
        if (s_confirm_focus == CONFIRM_EXIT) {
            exit_to_menu();
        } else {
            close_confirm();
        }
    }
    return true;
}

static bool on_input(const InputEvent* e) {
    if (s_confirm_open) return on_confirm_input(e);
    bool short_back = e->type == INPUT_EVT_SHORT_PRESS && e->key == INPUT_KEY_BACK;
    if (s_game.phase == SNAKE_PHASE_OVER) {
        if (!s_result_shown || e->type != INPUT_EVT_SHORT_PRESS) return true;
        if (e->key == INPUT_KEY_OK && lv_tick_elaps(s_result_at) >= RESULT_LOCK_MS) start();
        if (e->key == INPUT_KEY_BACK) page_replace(&page_snake_entry, NULL);
        return true;
    }
    if (short_back) {
        if (s_game.phase == SNAKE_PHASE_PAUSED) {
            exit_to_menu();
        } else {
            open_confirm();
        }
        return true;
    }
    if (e->type != INPUT_EVT_PRESS) return true;
    // 按键在两帧之间到达，先把规则推进到现在，转向才算进正确的节拍
    advance();
    if (e->key == INPUT_KEY_OK) snake_turn(&s_game);
    bool paused = e->key == INPUT_KEY_SELECT && snake_toggle_pause(&s_game);
    render();
    if (paused) UI_SFX_PLAY(SFX_PAUSE);
    return true;
}

const Page page_snake = {
    .name = "snake",
    .create = create,
    .destroy = destroy,
    .on_input = on_input,
    .idle_timeout_ms = 0,
};
