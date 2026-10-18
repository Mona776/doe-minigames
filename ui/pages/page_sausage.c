/**
 * Sausage Snake - 贪吃腊肠的无尽版：一条命，开局倒计时 3、2、1、GO，之后 Doe 自动前进，越吃越快、石头越来越多；
 * 从入口页（page_sausage_entry.c）替换过来
 * 按键：B（确定）顺时针转 90°（按下即生效，按住不重复），A（选择）暂停/继续，
 * C（返回）弹出结束确认（倒计时、走动时；弹窗打开时游戏冻结），暂停中按 C 直接结束；手动结束也按当前骨头数结算
 * 规则在 core/games/sausage.c；这里每帧推进规则，10×10 网格每格一个图片对象，美术、拼接和脚与贪吃腊肠一样，
 * 像素画共用 snake_art
 * 顶栏从左到右：档位、骨头数、三个星位、历史最高；吃到第 5、10、15 根时从骨头数那里飞出 5 根小骨头落进星位，
 * 最后一根落下时星位点亮弹一下，配一串越来越高的「嘀」加一声「叮」；之后的升档把最高分临时换成「加速!」；
 * 本局超过历史最高后最高分变亮、跟着骨头数走；
 * 点亮的星（顶栏和结算页）每隔一会儿有一道高光斜着扫过，三颗依次错开；
 * 新加的石头闪几下再定住；
 * 撞到后腊肠闪几下再显示结算，手动结束立即显示结算；结算把奖励和骨头数（最高分）作为命令发给逻辑服务；
 * 结算页选择键切换「再来一局 / 返回入口」，确认键执行；返回键无操作
 * 走动时循环放一段复古街机风的背景乐，档位越高放得越快；音效响时背景乐让开，暂停、结束确认时停在原处；
 * 音效是复古街机风格，走路和转向不出声：吃到骨头「啾」，倒计时「嘀、嘀、嘀、嘀——」，升档一串快速上行琶音，
 * 撞到先一下噪声再一路滑下去
 * 特效都按时间戳逐帧算：骨头单独一张图，出现时弹出来、平时上下浮动；吃到时炸出碎屑、分数弹一下；
 * 撞到时整个场地（放在一个容器里）左右震，头上转一圈星星，整条腊肠闪烁，撞到的石头或身体闪红、撞到的墙闪烁
 */

#include <stdio.h>
#include "pages.h"
#include "core/app_msg.h"
#include "core/games/sausage.h"
#include "hal/hal_clock.h"
#include "ui/ui_records.h"
#include "ui/ui_sfx.h"
#include "ui/ui_theme.h"
#include "game_art.h"
#include "snake_art.h"
#include "snake_ui.h"
#include "sausage_ui.h"

#define FRAME_MS       15
#define MAX_STEP_MS    100  // 卡顿时一帧最多推进这么久
#define RESULT_LOCK_MS 800  // 结算出现后这么久内不接受「再来」，防止连按误触
#define CRASH_MS       900  // 撞到后腊肠闪这么久再出结算
#define BLINK_MS       120
#define SCORE_FLASH_MS 400  // 吃到骨头时分数亮一下
#define TOAST_MS       1000 // 升档提示停留
#define ROCK_BLINK_MS  900  // 新石头闪这么久再定住
#define LEG_FRAMES_PER_STEP 4  // 每走一格脚换几次姿势

#define CELL_PX        20
#define TILE_SCALE     (CELL_PX / SNAKE_TILE)
#define FIELD_PX       (SAUSAGE_SIZE * CELL_PX)
#define FIELD_X        ((240 - FIELD_PX) / 2)
#define FIELD_Y        30
#define WALL_W         2
#define LEGS_PX        (SNAKE_LEGS * TILE_SCALE)
#define LEGS_INSET     ((LEGS_PX - CELL_PX) / 2)
#define DIGIT_SCALE    4

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
#define BEST_TOAST     0xFFFF
#define STAR_N         3
#define STAR_SCALE     2
#define STAR_PX        (SNAKE_STAR * STAR_SCALE)
#define STAR_TURN_MS   600  // 星星转一圈的时间
#define STAR_RX        11
#define STAR_RY        4
#define STAR_LIFT      13   // 星星转的圈离头中心多高

// 顶栏一行四项（档位、骨头数、星位、最高）两端和场地的墙对齐，中间的空隙平均分；
// 12px 像素字配紧凑图标，给五位分数和最高纪录留足空间。
#define HEADER_X       (FIELD_X - WALL_W)
#define HEADER_Y       9
#define HEADER_W       (FIELD_PX + 2 * WALL_W)
#define HEADER_H       18
#define HUD_BONE_SCALE 2
#define HUD_BONE_PX    (SNAKE_ICON * HUD_BONE_SCALE)
#define HUD_STAR_PX    9   // 星位按素材原尺寸显示，避免非整数缩放
#define HUD_STAR_GAP   2
#define FLY_BONES      5    // 换一颗星飞过去几根小骨头
#define FLY_GAP_MS     60
#define FLY_MS         300
#define FLY_DIP        14   // 小骨头飞的弧线往下弯多少
#define FLY_END_SCALE  128  // 落进星位时缩到多大（256 = 1 倍）
#define STAR_LIT_MS    ((FLY_BONES - 1) * FLY_GAP_MS + FLY_MS)  // 最后一根落下、星位点亮
#define STAR_POP_MS    300
#define STAR_POP_PEAK  110
#define SHINE_FRAMES   6     // 高光从左上扫到右下分几帧
#define SHINE_STEP     3     // 每帧高光往右下挪几条对角线
#define SHINE_FRAME_MS 50
#define SHINE_PERIOD_MS 2400 // 每隔这么久扫一次
#define SHINE_STAGGER_MS 150 // 三颗星依次错开

// 碎屑的初速度，像素/秒
static const int16_t DEBRIS_V[DEBRIS_N][2] = {
    {-90, -130}, {-30, -170}, {40, -160}, {100, -110},
};

static const UiNote SFX_EAT[] = {{1100, 22, 1700}};
static const UiNote SFX_COUNT[] = {{440, 90}};
static const UiNote SFX_GO[] = {{880, 70}, {0, 20}, {1760, 240}};
static const UiNote SFX_TIER_UP[] = {
    {523, 35}, {659, 35}, {784, 35}, {1047, 35},
    {659, 35}, {784, 35}, {1047, 35}, {1319, 35},
    {784, 35}, {1047, 35}, {1319, 35}, {1568, 35},
    {2093, 70}, {2217, 50}, {2093, 160},
};
// 吃到的「啾」，接着 5 根小骨头依次落进星位各「嘀」一声、越来越高，最后一声亮的「叮——」
static const UiNote SFX_STAR[] = {
    {1100, 22, 1700}, {0, FLY_MS - 22},
    {1047, 30}, {0, FLY_GAP_MS - 30}, {1175, 30}, {0, FLY_GAP_MS - 30}, {1319, 30}, {0, FLY_GAP_MS - 30},
    {1568, 30}, {0, FLY_GAP_MS - 30}, {1760, 30},
    {0, 20}, {2093, 60}, {2637, 280, 2794},
};
static const UiNote SFX_CRASH[] = {{90, 140, 480, true}, {700, 420, 70}};

// 背景乐：C 大调轻快的分解和弦旋律，16 小节循环；前 8 小节 C-Am-F-G 走两遍，后 8 小节 F-G-Em-Am-F-G-C-G7
#define BGM_8TH        180  // 原速一个八分音符的时长
#define BGM_TEMPO_STEP 4    // 每升一档快这么多（百分比）
#define BGM(hz, eighths) {(hz), (eighths) * BGM_8TH}
static const UiNote BGM_LOOP[] = {
    BGM(523, 1), BGM(659, 1), BGM(784, 1), BGM(659, 1), BGM(1047, 2), BGM(784, 2),
    BGM(880, 2), BGM(659, 2), BGM(523, 1), BGM(659, 1), BGM(880, 2),
    BGM(698, 2), BGM(880, 2), BGM(1047, 1), BGM(880, 1), BGM(698, 1), BGM(880, 1),
    BGM(784, 3), BGM(988, 1), BGM(1175, 2), BGM(988, 1), BGM(784, 1),
    BGM(523, 1), BGM(659, 1), BGM(784, 1), BGM(659, 1), BGM(1047, 2), BGM(1319, 2),
    BGM(1175, 1), BGM(1047, 1), BGM(880, 1), BGM(659, 1), BGM(880, 2), BGM(1047, 2),
    BGM(1047, 1), BGM(880, 1), BGM(698, 1), BGM(880, 1), BGM(988, 2), BGM(1175, 2),
    BGM(1047, 2), BGM(784, 1), BGM(659, 1), BGM(523, 2), BGM(0, 2),

    BGM(880, 2), BGM(1047, 2), BGM(880, 1), BGM(698, 1), BGM(880, 1), BGM(1047, 1),
    BGM(988, 2), BGM(1175, 2), BGM(988, 1), BGM(784, 1), BGM(988, 1), BGM(1175, 1),
    BGM(1319, 2), BGM(1175, 1), BGM(988, 1), BGM(784, 2), BGM(659, 2),
    BGM(880, 2), BGM(1047, 2), BGM(1319, 3), BGM(0, 1),
    BGM(698, 1), BGM(880, 1), BGM(1047, 1), BGM(880, 1), BGM(698, 1), BGM(880, 1), BGM(1047, 1), BGM(880, 1),
    BGM(784, 1), BGM(988, 1), BGM(1175, 1), BGM(988, 1), BGM(784, 1), BGM(988, 1), BGM(1175, 1), BGM(988, 1),
    BGM(1319, 2), BGM(1047, 2), BGM(784, 2), BGM(659, 2),
    BGM(587, 1), BGM(698, 1), BGM(784, 1), BGM(988, 1), BGM(1175, 2), BGM(784, 2),
};
static const UiNote SFX_PAUSE[] = {{1319, 35}, {0, 25}, {988, 45}};

#define COLOR_CELL_A   SAUSAGE_UI_CELL_A
#define COLOR_CELL_B   SAUSAGE_UI_CELL_B

// 图是朝右、肚子朝下画的：往左走镜像，竖着走转 90°，肚子和脚始终在下方或右侧
static const SpriteTurn TURN[] = {
    [SAUSAGE_UP]    = {3, false},
    [SAUSAGE_RIGHT] = {0, false},
    [SAUSAGE_DOWN]  = {1, true},
    [SAUSAGE_LEFT]  = {0, true},
};

#define ICON_SCALE       2
#define ICON_PX          (SNAKE_ICON * ICON_SCALE)
#define RESULT_STAR_SCALE 3
#define DIM_OPA          LV_OPA_60  // 暂停、结束确认时压暗场景

enum { CONFIRM_STAY, CONFIRM_END, CONFIRM_COUNT };
enum { RESULT_RETRY, RESULT_LEAVE, RESULT_COUNT };

static SausageGame s_game;
static lv_timer_t* s_timer;
static uint32_t s_last_tick;
static uint32_t s_over_at;
static uint32_t s_result_at;
static uint32_t s_score_at;
static uint32_t s_bone_at;
static uint32_t s_eat_at;
static uint32_t s_toast_at;
static uint32_t s_rocks_at;
static uint8_t s_eat_cell;
static bool s_result_shown;
static bool s_confirm_open;
static uint8_t s_confirm_focus;
static uint8_t s_result_focus;
static lv_obj_t* s_result_options[RESULT_COUNT];
static bool s_new_rock[SAUSAGE_CELLS];  // 刚升档加的石头，闪完之前要逐帧画
static bool s_rocks_blinking;
static uint8_t s_earned_stars;  // 已经拿到的星
static uint8_t s_lit_stars;     // 已经点亮的星位，小骨头飞完之前比拿到的少一颗
static uint32_t s_star_at;      // 最近一颗星开始换的时刻
static int32_t s_star_scale;

// 界面上次画的状态，变了才重画
static uint16_t s_shown_moves;
static uint8_t s_shown_bone;
static SausagePhase s_shown_phase;
static uint16_t s_shown_bones;
static uint8_t s_shown_tier;
static uint8_t s_shown_rock_segments;
static bool s_shown_blocked[SAUSAGE_CELLS];
static uint8_t s_shown_count;
static const void* s_cell_src[SAUSAGE_CELLS];
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
static lv_draw_buf_t* s_front_buf;
static lv_draw_buf_t* s_back_buf;
static lv_draw_buf_t* s_digit_buf;
static lv_draw_buf_t* s_star_buf;
static lv_draw_buf_t* s_icon_bone_buf;
static lv_draw_buf_t* s_icon_shell_buf;
static lv_draw_buf_t* s_icon_star_buf[2];
static lv_draw_buf_t* s_shine_buf[SHINE_FRAMES];  // 亮星加一道斜高光，按帧

static lv_obj_t* s_field;  // 墙、格子、脚和特效都在这里面，震动时只挪它
static lv_obj_t* s_wall;
static lv_obj_t* s_cells[SAUSAGE_CELLS];
static lv_obj_t* s_bone;
static lv_obj_t* s_hit;    // 撞到的那格上闪的红块
static lv_obj_t* s_debris[DEBRIS_N];
static lv_obj_t* s_stars[STAR_N];
static Legs s_front;
static Legs s_back;
static lv_obj_t* s_header;
static lv_obj_t* s_tier;
static lv_obj_t* s_best;   // 历史最高，本局超过后跟着骨头数走；升档时临时换成「加速!」
static uint16_t s_start_best;  // 开局时的历史最高
static uint16_t s_shown_best;  // 上次画的数字，显示「加速!」时是 BEST_TOAST
static bool s_record_shown;    // 「破纪录」的样式已经亮出来了
static uint32_t s_record_at;
static lv_obj_t* s_score_box;  // 骨头图标 + 骨头数，吃到时整体弹一下
static lv_obj_t* s_score;
static lv_obj_t* s_hud_stars[3];
static lv_obj_t* s_fly_bones[FLY_BONES];
static lv_obj_t* s_digit;
static lv_obj_t* s_go;
static lv_obj_t* s_pause;
static lv_obj_t* s_confirm;
static lv_obj_t* s_confirm_options[CONFIRM_COUNT];
static lv_obj_t* s_result;
static lv_obj_t* s_result_bones;
static lv_obj_t* s_result_stars[3];
static lv_obj_t* s_result_shells;
static lv_obj_t* s_result_best;

static lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static SausageDir dir_between(uint8_t from, uint8_t to) {
    int d = (int)to - (int)from;
    if (d == 1) return SAUSAGE_RIGHT;
    if (d == -1) return SAUSAGE_LEFT;
    return d > 0 ? SAUSAGE_DOWN : SAUSAGE_UP;
}

// 尾巴那一节朝哪走（朝它前面一节）
static SausageDir tail_dir(const SausageGame* g) {
    return dir_between(sausage_segment(g, g->len - 1), sausage_segment(g, g->len - 2));
}

static const void* segment_tile(const SausageGame* g, uint8_t i) {
    uint8_t c = sausage_segment(g, i);
    if (i == 0) return s_head_buf[sausage_next_dir(g)];
    if (i == g->len - 1) return s_tail_buf[tail_dir(g)];
    SausageDir to_head = dir_between(c, sausage_segment(g, i - 1));
    SausageDir to_tail = dir_between(c, sausage_segment(g, i + 1));
    if ((to_head + 2) % 4 == to_tail) return s_body_buf[to_head % 2 ? 0 : 1];
    // 转角图接的是下、左两边，即相邻两个方向里靠前的是「下」，顺时针转到实际的那一对
    SausageDir first = (to_tail - to_head + 4) % 4 == 1 ? to_head : to_tail;
    return s_corner_buf[(first + 2) % 4];
}

static void set_cell(uint8_t c, const void* src) {
    if (s_cell_src[c] == src) return;
    s_cell_src[c] = src;
    lv_image_set_src(s_cells[c], src);
}

static void draw_field(const SausageGame* g) {
    const void* src[SAUSAGE_CELLS] = {0};
    for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) {
        if (g->blocked[c]) src[c] = s_rock_buf;
    }
    for (uint8_t i = 0; i < g->len; i++) src[sausage_segment(g, i)] = segment_tile(g, i);
    for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) set_cell(c, src[c]);
}

static void cell_origin(uint8_t c, int32_t* x, int32_t* y) {
    *x = FIELD_X + (c % SAUSAGE_SIZE) * CELL_PX;
    *y = FIELD_Y + (c / SAUSAGE_SIZE) * CELL_PX;
}

static void place_legs(Legs* legs, uint8_t c, SausageDir dir, int8_t frame) {
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
static void render_legs(const SausageGame* g) {
    int8_t frame = 0;
    if (g->phase == SAUSAGE_PHASE_RUN) {
        frame = (int8_t)((g->phase_ms * LEG_FRAMES_PER_STEP / sausage_step_ms(g->tier)) % 2);
    }
    place_legs(&s_front, sausage_segment(g, 0), sausage_next_dir(g), frame);
    place_legs(&s_back, sausage_segment(g, g->len - 1), tail_dir(g),
               g->phase == SAUSAGE_PHASE_RUN ? 1 - frame : 0);
}

static void show(lv_obj_t* obj, bool shown) {
    if (lv_obj_is_hidden(obj) == shown) lv_obj_set_hidden(obj, !shown);
}

static void cell_center(uint8_t c, int32_t* x, int32_t* y) {
    cell_origin(c, x, y);
    *x += CELL_PX / 2;
    *y += CELL_PX / 2;
}

static uint8_t cell_ahead(uint8_t c, SausageDir dir) {
    switch (dir) {
        case SAUSAGE_UP:   return (uint8_t)(c - SAUSAGE_SIZE);
        case SAUSAGE_DOWN: return (uint8_t)(c + SAUSAGE_SIZE);
        case SAUSAGE_LEFT: return (uint8_t)(c - 1);
        default:           return (uint8_t)(c + 1);
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

// 骨头被身体压住（刚吃掉、场地满了）就不显示
static void render_bone(const SausageGame* g, uint32_t now) {
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

// 升档新加的石头：先一闪一闪，闪完定住
static void render_new_rocks(const SausageGame* g, uint32_t now) {
    if (g->rock_segments != s_shown_rock_segments) {
        s_shown_rock_segments = g->rock_segments;
        for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) {
            s_new_rock[c] = g->blocked[c] && !s_shown_blocked[c];
            s_shown_blocked[c] = g->blocked[c];
        }
        s_rocks_at = now;
        s_rocks_blinking = true;
    }
    if (!s_rocks_blinking) return;
    uint32_t t = lv_tick_diff(now, s_rocks_at);
    bool done = t >= ROCK_BLINK_MS;
    bool on = done || (t / BLINK_MS) % 2 == 1;
    for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) {
        if (s_new_rock[c]) set_cell(c, on ? s_rock_buf : NULL);
    }
    if (done) s_rocks_blinking = false;
}

static void show_snake(const SausageGame* g, bool shown) {
    for (uint8_t i = 0; i < g->len; i++) set_cell(sausage_segment(g, i), shown ? segment_tile(g, i) : NULL);
    show(s_front.canvas, shown);
    show(s_back.canvas, shown);
}

static bool crash_end(SausageEnd end) {
    return end == SAUSAGE_END_WALL || end == SAUSAGE_END_OBSTACLE || end == SAUSAGE_END_SELF;
}

// 撞到：场地左右震、越震越轻；头上转星星；整条腊肠一闪一闪，撞到的石头或身体跟着闪红；
// 撞墙时墙也跟着闪，结算出来后还接着闪
static void render_crash(const SausageGame* g, uint32_t now) {
    bool crashed = g->phase == SAUSAGE_PHASE_OVER && crash_end(g->end) && !s_result_shown;
    uint32_t t = crashed ? lv_tick_diff(now, s_over_at) : 0;

    int32_t dx = 0;
    if (crashed && t < SHAKE_MS) {
        int32_t amp = (SHAKE_PX * (int32_t)(SHAKE_MS - t) + SHAKE_MS - 1) / SHAKE_MS;
        dx = (t / SHAKE_HALF_MS) % 2 ? -amp : amp;
    }
    if (lv_obj_get_x(s_field) != dx) lv_obj_set_x(s_field, dx);

    bool on = crashed && (t / BLINK_MS) % 2 == 0;
    if (crashed) show_snake(g, on);
    bool wall_crash = g->phase == SAUSAGE_PHASE_OVER && g->end == SAUSAGE_END_WALL;
    show(s_wall, !wall_crash || (lv_tick_diff(now, s_over_at) / BLINK_MS) % 2 == 0);
    bool hit = on && g->end != SAUSAGE_END_WALL;
    show(s_hit, hit);
    uint8_t head = sausage_segment(g, 0);
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

static void render_countdown(const SausageGame* g) {
    uint8_t n = g->phase == SAUSAGE_PHASE_COUNTDOWN ? sausage_countdown(g) : 0xFF;
    if (n == s_shown_count) return;
    s_shown_count = n;
    lv_obj_set_hidden(s_header, g->phase == SAUSAGE_PHASE_COUNTDOWN);
    lv_obj_set_hidden(s_digit, n == 0 || n == 0xFF);
    lv_obj_set_hidden(s_go, n != 0);
    if (n && n != 0xFF) sprite_draw(s_digit, &game_digits[n - 1], DIGIT_SCALE);
    if (n == 0) UI_SFX_PLAY(SFX_GO);
    if (n && n != 0xFF) UI_SFX_PLAY(SFX_COUNT);
}

// 档位变了：更新顶栏；拿星以外的升档把最高分临时换成「加速!」；返回是否升档
static bool render_tier(const SausageGame* g, uint32_t now, bool star) {
    if (g->tier == s_shown_tier) return false;
    bool tier_up = s_shown_tier != 0;
    s_shown_tier = g->tier;
    lv_label_set_text_fmt(s_tier, "LV %u", (unsigned)g->tier);
    ui_music_tempo((uint16_t)(100 + BGM_TEMPO_STEP * (g->tier - 1)));
    if (tier_up && !star) s_toast_at = now;
    return tier_up;
}

static void obj_center(lv_obj_t* obj, int32_t* x, int32_t* y) {
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    *x = (a.x1 + a.x2) / 2;
    *y = (a.y1 + a.y2) / 2;
}

static void set_scale(lv_obj_t* obj, int32_t scale) {
    if (lv_obj_get_style_transform_scale_x(obj, 0) != scale) lv_obj_set_style_transform_scale(obj, scale, 0);
}

// 第 i 颗亮星现在该显示哪一帧：大部分时间是普通的亮星，每个周期里扫一遍高光
static const void* lit_star_src(uint32_t now, uint8_t i) {
    uint32_t t = (now + SHINE_PERIOD_MS - i * SHINE_STAGGER_MS) % SHINE_PERIOD_MS;
    uint32_t frame = t / SHINE_FRAME_MS;
    return frame < SHINE_FRAMES ? (const void*)s_shine_buf[frame] : (const void*)s_icon_star_buf[1];
}

static void set_src(lv_obj_t* img, const void* src) {
    if (lv_image_get_src(img) != src) lv_image_set_src(img, src);
}

// 顶栏和结算页上点亮的星都扫高光
static void render_shine(uint32_t now) {
    for (uint8_t i = 0; i < s_lit_stars; i++) set_src(s_hud_stars[i], lit_star_src(now, i));
    if (!s_result_shown) return;
    uint8_t stars = sausage_reward(s_game.bones).stars;
    for (uint8_t i = 0; i < stars; i++) set_src(s_result_stars[i], lit_star_src(now, i));
}

// 拿到星：骨头数那里依次飞出几根小骨头，沿往下弯的弧线落进星位、越飞越小；最后一根落下时星位点亮、弹一下。
// 返回这一帧是否刚拿到星
static bool render_stars(const SausageGame* g, uint32_t now) {
    uint8_t earned = sausage_reward(g->bones).stars;
    bool gained = earned > s_earned_stars;
    if (gained) {
        s_earned_stars = earned;
        s_star_at = now;
    }
    if (s_earned_stars == 0) return false;
    uint8_t slot = s_earned_stars - 1;
    uint32_t t = lv_tick_diff(now, s_star_at);
    if (s_lit_stars < s_earned_stars && t >= STAR_LIT_MS) s_lit_stars = s_earned_stars;
    bool flying = s_lit_stars < s_earned_stars;

    int32_t sx, sy, tx, ty;
    obj_center(lv_obj_get_child(s_score_box, 0), &sx, &sy);
    obj_center(s_hud_stars[slot], &tx, &ty);
    for (uint8_t i = 0; i < FLY_BONES; i++) {
        uint32_t start = i * FLY_GAP_MS;
        bool shown = flying && t >= start && t - start < FLY_MS;
        show(s_fly_bones[i], shown);
        if (!shown) continue;
        int32_t f = (int32_t)((t - start) * 256 / FLY_MS);
        int32_t x = sx + (tx - sx) * f / 256;
        int32_t y = sy + (ty - sy) * f / 256 + FLY_DIP * 4 * f * (256 - f) / 65536;
        lv_obj_set_pos(s_fly_bones[i], x - HUD_BONE_PX / 2, y - HUD_BONE_PX / 2);
        set_scale(s_fly_bones[i], 256 - (256 - FLY_END_SCALE) * f / 256);
    }

    int32_t scale = flying ? 256 : pop_scale(t - STAR_LIT_MS, STAR_POP_MS, 256, STAR_POP_PEAK);
    if (scale != s_star_scale) {
        s_star_scale = scale;
        set_scale(s_hud_stars[slot], scale);
    }
    return gained;
}

// 顶栏最右一项：平时是开局时的历史最高；本局超过以后变亮、数字跟着骨头数走，刚亮出来时弹一下；
// 升档时临时换成「加速!」，破纪录的那一下等「加速!」收起后再弹
static void render_best(const SausageGame* g, uint32_t now) {
    bool toast = lv_tick_diff(now, s_toast_at) < TOAST_MS;
    bool record = g->bones > s_start_best;
    if (record && !toast && !s_record_shown) {
        s_record_shown = true;
        s_record_at = now;
    }
    uint16_t value = toast ? BEST_TOAST : record ? g->bones : s_start_best;
    if (value != s_shown_best) {
        s_shown_best = value;
        if (toast) {
            lv_label_set_text(s_best, "加速!");
        } else {
            lv_label_set_text_fmt(s_best, "BEST %u", (unsigned)value);
        }
        lv_obj_set_style_text_color(s_best, lv_color_hex(toast || record ? SAUSAGE_UI_GOLD : SAUSAGE_UI_DIM), 0);
    }
    set_scale(s_best, pop_scale(lv_tick_diff(now, s_record_at), SCORE_POP_MS, 256, SCORE_POP_PEAK));
}

// 背景乐只在走动时放：倒计时完开始，暂停时停在原处，结束时停掉
static void update_music(SausagePhase phase, SausagePhase from) {
    if (phase == SAUSAGE_PHASE_RUN) {
        if (from == SAUSAGE_PHASE_PAUSED) {
            ui_music_pause(false);
        } else {
            UI_MUSIC_PLAY(BGM_LOOP);
        }
    } else if (phase == SAUSAGE_PHASE_PAUSED) {
        ui_music_pause(true);
    } else {
        ui_music_stop();
    }
}

// 同一帧里有几件事时只响最重要的：结束、拿星、升档、吃到骨头
static void play_sfx(const SausageGame* g, bool entered, bool star, bool tier_up, bool ate) {
    if (entered && g->phase == SAUSAGE_PHASE_OVER) {
        if (crash_end(g->end)) UI_SFX_PLAY(SFX_CRASH);
    } else if (star) {
        UI_SFX_PLAY(SFX_STAR);
    } else if (tier_up) {
        UI_SFX_PLAY(SFX_TIER_UP);
    } else if (ate) {
        UI_SFX_PLAY(SFX_EAT);
    }
}

static void render(void) {
    const SausageGame* g = &s_game;
    uint32_t now = lv_tick_get();
    bool star = render_stars(g, now);
    render_shine(now);
    bool tier_up = render_tier(g, now, star);
    render_countdown(g);

    bool ate = g->bones > s_shown_bones && s_shown_bones != 0xFFFF;
    bool phase_changed = g->phase != s_shown_phase;
    if (phase_changed) {
        update_music(g->phase, s_shown_phase);
        s_shown_phase = g->phase;
        lv_obj_set_hidden(s_pause, g->phase != SAUSAGE_PHASE_PAUSED);
        if (g->phase == SAUSAGE_PHASE_OVER) s_over_at = now;
    }
    if (g->bone != s_shown_bone) s_bone_at = now;
    if (phase_changed || g->moves != s_shown_moves || g->bone != s_shown_bone) {
        s_shown_moves = g->moves;
        s_shown_bone = g->bone;
        draw_field(g);
    }
    render_new_rocks(g, now);
    render_bone(g, now);
    render_legs(g);
    render_best(g, now);

    if (ate) {
        s_eat_at = now;
        s_eat_cell = sausage_segment(g, 0);
    }
    render_eat(now);
    render_crash(g, now);

    if (g->bones != s_shown_bones) {
        if (ate) s_score_at = now;
        s_shown_bones = g->bones;
        lv_label_set_text_fmt(s_score, "%u", (unsigned)g->bones);
    }
    bool flash = lv_tick_diff(now, s_score_at) < SCORE_FLASH_MS;
    lv_obj_set_style_text_color(s_score, lv_color_hex(flash ? SAUSAGE_UI_GOLD : SAUSAGE_UI_TEXT), 0);
    play_sfx(g, phase_changed, star, tier_up, ate);
}

static const char* end_name(SausageEnd end) {
    switch (end) {
        case SAUSAGE_END_WALL:     return "撞墙了";
        case SAUSAGE_END_OBSTACLE: return "撞到石头了";
        case SAUSAGE_END_SELF:     return "咬到自己了";
        case SAUSAGE_END_QUIT:     return "手动结束";
        case SAUSAGE_END_FULL:     return "场地满了";
        default:                   return "";
    }
}

static void set_result_focus(uint8_t focus) {
    s_result_focus = focus;
    for (uint8_t i = 0; i < RESULT_COUNT; i++) sausage_ui_focus(s_result_options[i], i == focus);
}

static void show_result(void) {
    const SausageGame* g = &s_game;
    SausageReward reward = sausage_reward(g->bones);
    uint16_t best = ui_records_best(GAME_SAUSAGE);
    bool new_record = g->bones > best;
    AppCmd cmd = {
        .type = APP_CMD_GAME_RESULT,
        .result = {.game = GAME_SAUSAGE, .stars = reward.stars, .shells = reward.shells, .spirit = reward.spirit,
                   .score = g->bones},
    };
    app_cmd_send(&cmd);

    lv_label_set_text_fmt(s_result_bones, "%u", (unsigned)g->bones);
    for (uint8_t i = 0; i < 3; i++) lv_image_set_src(s_result_stars[i], s_icon_star_buf[i < reward.stars]);
    lv_label_set_text_fmt(s_result_shells, "+%u", (unsigned)reward.shells);
    if (new_record) {
        lv_label_set_text_fmt(s_result_best, "新纪录! %u", (unsigned)g->bones);
    } else {
        lv_label_set_text_fmt(s_result_best, "最高纪录 %u", (unsigned)best);
    }
    lv_obj_set_style_text_color(s_result_best, lv_color_hex(new_record ? SAUSAGE_UI_GOLD : SAUSAGE_UI_DIM), 0);
    set_result_focus(RESULT_RETRY);
    lv_obj_set_hidden(s_result, false);
    show_snake(g, true);

    s_result_shown = true;
    s_result_at = lv_tick_get();
    printf("[Sausage] %s，第 %u 档，吃到 %u 根，%u 星%s，用时 %.1f 秒\n", end_name(g->end), (unsigned)g->tier,
           (unsigned)g->bones, (unsigned)reward.stars, new_record ? "，新纪录" : "", g->elapsed_ms / 1000.0);
}

static void advance(void) {
    uint32_t now = lv_tick_get();
    uint32_t dt = now - s_last_tick;
    s_last_tick = now;
    sausage_step(&s_game, dt > MAX_STEP_MS ? MAX_STEP_MS : dt);
}

static void on_frame(lv_timer_t* timer) {
    (void)timer;
    if (!s_confirm_open) advance();
    render();
    if (s_game.phase == SAUSAGE_PHASE_OVER && !s_result_shown && !s_confirm_open) {
        uint32_t wait = crash_end(s_game.end) ? CRASH_MS : 0;
        if (lv_tick_elaps(s_over_at) >= wait) show_result();
    }
}

static void start(void) {
    sausage_init(&s_game, (uint32_t)hal_clock_us());
    s_last_tick = lv_tick_get();
    s_result_shown = false;
    s_shown_moves = 0xFFFF;
    s_shown_phase = SAUSAGE_PHASE_OVER;
    s_shown_bones = 0xFFFF;
    s_shown_tier = 0;
    s_shown_rock_segments = s_game.rock_segments;
    for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) s_shown_blocked[c] = s_game.blocked[c];
    s_rocks_blinking = false;
    s_score_at = s_last_tick - SCORE_FLASH_MS;
    s_eat_at = s_last_tick - DEBRIS_MS;
    s_toast_at = s_last_tick - TOAST_MS;
    s_shown_bone = 0xFF;
    s_shown_count = 0xFE;
    s_start_best = ui_records_best(GAME_SAUSAGE);
    s_shown_best = BEST_TOAST - 1;
    s_record_shown = false;
    s_record_at = s_last_tick - SCORE_POP_MS;
    s_earned_stars = 0;
    s_lit_stars = 0;
    s_star_scale = 256;
    for (uint8_t i = 0; i < 3; i++) {
        lv_image_set_src(s_hud_stars[i], s_icon_star_buf[0]);
        set_scale(s_hud_stars[i], 256);
    }
    for (uint8_t i = 0; i < FLY_BONES; i++) lv_obj_set_hidden(s_fly_bones[i], true);
    s_confirm_open = false;
    lv_obj_set_hidden(s_confirm, true);
    lv_obj_set_hidden(s_result, true);
    render();
}

// 亮星的高光帧：把亮星上落在两条相邻对角线（x + y = k、k + 1）上的像素换成奶油白，k 每帧往右下挪
static void create_shine(void) {
    static char rows[SHINE_FRAMES][SNAKE_STAR_ICON][SNAKE_STAR_ICON + 1];
    static const char* row_ptrs[SHINE_FRAMES][SNAKE_STAR_ICON];
    static Sprite frames[SHINE_FRAMES];
    const Sprite* lit = &snake_icon_star[1];
    for (uint8_t f = 0; f < SHINE_FRAMES; f++) {
        int k = f * SHINE_STEP;
        for (int y = 0; y < SNAKE_STAR_ICON; y++) {
            for (int x = 0; x < SNAKE_STAR_ICON; x++) {
                char c = lit->rows[y][x];
                rows[f][y][x] = c != '.' && (x + y == k || x + y == k + 1) ? 'W' : c;
            }
            rows[f][y][SNAKE_STAR_ICON] = '\0';
            row_ptrs[f][y] = rows[f][y];
        }
        frames[f] = (Sprite){row_ptrs[f], SNAKE_STAR_ICON};
        sprite_buf_render(&s_shine_buf[f], &frames[f], 1, (SpriteTurn){0, false});
    }
}

static void create_tiles(void) {
    for (uint8_t r = 0; r < 4; r++) {
        sprite_buf_render(&s_head_buf[r], &snake_head, TILE_SCALE, TURN[r]);
        sprite_buf_render(&s_tail_buf[r], &snake_tail, TILE_SCALE, TURN[r]);
        sprite_buf_render(&s_corner_buf[r], &snake_corner, TILE_SCALE, (SpriteTurn){r, false});
    }
    sprite_buf_render(&s_body_buf[0], &snake_body, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_body_buf[1], &snake_body, TILE_SCALE, (SpriteTurn){1, false});
    sprite_buf_render(&s_bone_buf, &snake_bone, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_rock_buf, &snake_rock, TILE_SCALE, (SpriteTurn){0, false});
    sprite_buf_render(&s_star_buf, &snake_star, STAR_SCALE, (SpriteTurn){0, false});
    for (uint8_t i = 0; i < 2; i++) {
        sprite_buf_render(&s_icon_star_buf[i], &snake_icon_star[i], 1, (SpriteTurn){0, false});
    }
    create_shine();
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
    lv_obj_set_style_border_color(s_wall, lv_color_hex(SAUSAGE_UI_JADE), 0);
    // 棋盘格底色方便看清格子
    for (uint8_t c = 0; c < SAUSAGE_CELLS; c++) {
        int32_t x, y;
        cell_origin(c, &x, &y);
        lv_obj_t* cell = lv_image_create(field);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, CELL_PX, CELL_PX);
        bool dark = (c % SAUSAGE_SIZE + c / SAUSAGE_SIZE) % 2 == 0;
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

static lv_obj_t* flex_row(lv_obj_t* parent, int32_t gap) {
    lv_obj_t* row = ui_box_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, gap, 0);
    return row;
}

// 一行：图标 + 文字，返回文字
static lv_obj_t* icon_row(lv_obj_t* parent, lv_obj_t** row_out, lv_draw_buf_t** buf, const Sprite* icon,
                          int32_t icon_scale, const lv_font_t* font, uint32_t color) {
    lv_obj_t* row = flex_row(parent, 4);
    snake_icon_create(row, buf, icon, icon_scale);
    if (row_out) *row_out = row;
    return label(row, font, color);
}

static void create_header(lv_obj_t* scr) {
    lv_obj_t* bar = s_header = ui_box_create(scr);
    lv_obj_set_pos(bar, HEADER_X, HEADER_Y);
    lv_obj_set_size(bar, HEADER_W, HEADER_H);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_tier = label(bar, &font_ui_12, SAUSAGE_UI_GOLD);
    s_score = icon_row(bar, &s_score_box, &s_icon_bone_buf, &snake_icon_bone, HUD_BONE_SCALE, &font_ui_12,
                       SAUSAGE_UI_TEXT);
    lv_obj_set_style_transform_pivot_x(s_score_box, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(s_score_box, LV_PCT(50), 0);
    s_score_scale = 256;

    lv_obj_t* stars = flex_row(bar, HUD_STAR_GAP);
    for (uint8_t i = 0; i < 3; i++) {
        s_hud_stars[i] = snake_icon_create(stars, &s_icon_star_buf[0], &snake_icon_star[0], 1);
        lv_obj_set_size(s_hud_stars[i], HUD_STAR_PX, HUD_STAR_PX);
        lv_obj_set_style_transform_pivot_x(s_hud_stars[i], LV_PCT(50), 0);
        lv_obj_set_style_transform_pivot_y(s_hud_stars[i], LV_PCT(50), 0);
    }
    s_best = label(bar, &font_ui_12, SAUSAGE_UI_DIM);
    lv_obj_set_style_transform_pivot_x(s_best, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(s_best, LV_PCT(50), 0);
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
    lv_obj_t* panel = sausage_ui_panel(s_confirm, 184, 154);
    lv_obj_set_pos(panel, 28, 41);
    lv_obj_t* line = label(s_confirm, &font_ui_24, SAUSAGE_UI_TEXT);
    lv_label_set_text(line, "结束本局？");
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_t* shell = snake_icon_create(s_confirm, &s_icon_shell_buf, &snake_icon_shell, ICON_SCALE);
    lv_obj_align(shell, LV_ALIGN_TOP_MID, 0, 80);
    line = label(s_confirm, &font_ui_12, SAUSAGE_UI_DIM);
    lv_label_set_text(line, "按当前成绩结算");
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 102);
    static const char* const text[CONFIRM_COUNT] = {"继续游戏", "结束本局"};
    for (uint8_t i = 0; i < CONFIRM_COUNT; i++) {
        s_confirm_options[i] = sausage_ui_option(s_confirm, text[i], &font_ui_12, 142, 26);
        lv_obj_set_pos(s_confirm_options[i], 49, 127 + i * 34);
    }
    sausage_ui_hint(s_confirm, "A 切换  B 确定  C 取消");
}

static void create_result_doe(lv_obj_t* parent) {
    lv_obj_t* doe = sausage_ui_doe(parent, 2);
    lv_obj_align(doe, LV_ALIGN_TOP_MID, 0, 49);
}

static void create_overlays(lv_obj_t* scr) {
    for (uint8_t i = 0; i < FLY_BONES; i++) {
        s_fly_bones[i] = snake_icon_create(scr, &s_icon_bone_buf, &snake_icon_bone, HUD_BONE_SCALE);
        lv_obj_set_style_transform_pivot_x(s_fly_bones[i], LV_PCT(50), 0);
        lv_obj_set_style_transform_pivot_y(s_fly_bones[i], LV_PCT(50), 0);
        lv_obj_set_hidden(s_fly_bones[i], true);
    }
    s_digit = sprite_canvas_create(scr, &s_digit_buf, GAME_DIGIT_W * DIGIT_SCALE, GAME_DIGIT_H * DIGIT_SCALE);
    lv_obj_align(s_digit, LV_ALIGN_TOP_MID, 0, 64);
    s_go = label(scr, &font_ui_24, SAUSAGE_UI_GOLD);
    lv_label_set_text(s_go, "GO");
    lv_obj_align(s_go, LV_ALIGN_TOP_MID, 0, 70);

    s_pause = dim_layer(scr);
    lv_obj_t* panel = sausage_ui_panel(s_pause, 170, 90);
    lv_obj_set_pos(panel, 35, 75);
    lv_obj_t* line = label(s_pause, &font_ui_24, SAUSAGE_UI_TEXT);
    lv_label_set_text(line, "暂停");
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 87);
    lv_obj_t* pause_keys = sausage_ui_keys(s_pause, "A 继续  C 结束");
    lv_obj_align(pause_keys, LV_ALIGN_TOP_MID, 0, 132);

    // 结算与入口一致：选择键切换，确认键执行，返回键不参与。
    s_result = ui_box_create(scr);
    lv_obj_set_size(s_result, 240, 240);
    sausage_ui_background(s_result);
    lv_obj_t* title = label(s_result, &font_ui_24, SAUSAGE_UI_TEXT);
    lv_label_set_text(title, "本局结束");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);
    create_result_doe(s_result);
    lv_obj_t* stars = flex_row(s_result, 8);
    lv_obj_align(stars, LV_ALIGN_TOP_MID, 0, 83);
    for (uint8_t i = 0; i < 3; i++) {
        s_result_stars[i] = snake_icon_create(stars, &s_icon_star_buf[0], &snake_icon_star[0], RESULT_STAR_SCALE);
    }
    lv_obj_t* rewards = flex_row(s_result, 20);
    lv_obj_align(rewards, LV_ALIGN_TOP_MID, 0, 106);
    s_result_bones =
        icon_row(rewards, NULL, &s_icon_bone_buf, &snake_icon_bone, ICON_SCALE, &font_ui_24, SAUSAGE_UI_TEXT);
    s_result_shells =
        icon_row(rewards, NULL, &s_icon_shell_buf, &snake_icon_shell, ICON_SCALE, &font_ui_24, SAUSAGE_UI_TEXT);
    s_result_best = label(s_result, &font_ui_12, SAUSAGE_UI_DIM);
    lv_obj_align(s_result_best, LV_ALIGN_TOP_MID, 0, 132);
    static const char* const result_text[RESULT_COUNT] = {"再来一局", "返回入口"};
    for (uint8_t i = 0; i < RESULT_COUNT; i++) {
        s_result_options[i] = sausage_ui_option(s_result, result_text[i], &font_ui_12, 142, 28);
        lv_obj_set_pos(s_result_options[i], 49, 146 + i * 32);
    }
    lv_obj_t* result_keys = sausage_ui_keys(s_result, "A 切换  B 确定");
    lv_obj_align(result_keys, LV_ALIGN_TOP_MID, 0, 212);

    create_confirm(scr);
}

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    sausage_ui_screen(scr, false);
    create_tiles();
    create_header(scr);
    create_field(scr);
    create_overlays(scr);

    start();
    s_timer = lv_timer_create(on_frame, FRAME_MS, NULL);
}

static void destroy(void) {
    ui_music_stop();
    if (s_timer) lv_timer_delete(s_timer);
    s_timer = NULL;
}

static void set_confirm_focus(uint8_t focus) {
    s_confirm_focus = focus;
    for (uint8_t i = 0; i < CONFIRM_COUNT; i++) sausage_ui_focus(s_confirm_options[i], i == focus);
}

// 打开时先把规则推进到现在再冻结；关掉后从关掉的时刻接着算，弹窗开着的时间不算进游戏
static void open_confirm(void) {
    advance();
    // 推进到现在时刚好撞到，就走撞到的结算，不再弹确认
    if (s_game.phase == SAUSAGE_PHASE_OVER) {
        render();
        return;
    }
    s_confirm_open = true;
    set_confirm_focus(CONFIRM_STAY);
    lv_obj_set_hidden(s_confirm, false);
    ui_music_pause(true);
}

static void close_confirm(void) {
    s_confirm_open = false;
    s_last_tick = lv_tick_get();
    lv_obj_set_hidden(s_confirm, true);
    ui_music_pause(false);
}

// 手动结束：按现在的骨头数立即结算
static void end_game(void) {
    if (!sausage_quit(&s_game)) return;
    render();
    show_result();
}

static bool on_confirm_input(const InputEvent* e) {
    if (e->type != INPUT_EVT_SHORT_PRESS) return true;
    if (e->key == INPUT_KEY_SELECT) set_confirm_focus((uint8_t)((s_confirm_focus + 1) % CONFIRM_COUNT));
    if (e->key == INPUT_KEY_BACK) close_confirm();
    if (e->key == INPUT_KEY_OK) {
        close_confirm();
        if (s_confirm_focus == CONFIRM_END) end_game();
    }
    return true;
}

static bool on_input(const InputEvent* e) {
    if (s_confirm_open) return on_confirm_input(e);
    bool short_back = e->type == INPUT_EVT_SHORT_PRESS && e->key == INPUT_KEY_BACK;
    if (s_game.phase == SAUSAGE_PHASE_OVER) {
        if (!s_result_shown || e->type != INPUT_EVT_SHORT_PRESS) return true;
        if (e->key == INPUT_KEY_SELECT) set_result_focus((uint8_t)((s_result_focus + 1) % RESULT_COUNT));
        if (e->key == INPUT_KEY_OK && lv_tick_elaps(s_result_at) >= RESULT_LOCK_MS) {
            if (s_result_focus == RESULT_RETRY) start();
            else page_replace(&page_sausage_entry, NULL);
        }
        return true;
    }
    if (short_back) {
        if (s_game.phase == SAUSAGE_PHASE_PAUSED) {
            end_game();
        } else {
            open_confirm();
        }
        return true;
    }
    if (e->type != INPUT_EVT_PRESS) return true;
    // 先完成输入处理前已经到时的移动，再更新所选方向，不回滚路径
    advance();
    if (e->key == INPUT_KEY_OK) sausage_turn(&s_game);
    bool paused = e->key == INPUT_KEY_SELECT && sausage_toggle_pause(&s_game);
    render();
    if (paused) UI_SFX_PLAY(SFX_PAUSE);
    return true;
}

const Page page_sausage = {
    .name = "sausage",
    .create = create,
    .destroy = destroy,
    .on_input = on_input,
    .idle_timeout_ms = 0,
};
