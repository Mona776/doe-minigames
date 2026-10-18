/**
 * Doe 接网球 - 有透视的训练场：发球机在左边远处的地平线上，Doe 在右边近处，面朝发球机
 * 开局倒计时 3、2、1（上方显示玩法），之后发球机一球一球地发：亮灯、「滴」一声、抖一下，再「嘭」地把球弹出
 * 球从远处飞向镜头，越近越大、在屏幕上越飞越快，身后带一点拖尾，地上有球的影子
 * 确定键让 Doe 起跳张嘴（按下即生效，动作做完前再按无效），选择键暂停/继续，返回键中途退出、没有奖励
 * 规则在 core/games/tennis.c；这里每帧推进规则，按规则给出的进度摆放球、Doe 和发球机
 * 叼住后球留在嘴里、显示 Perfect!/Nice!，落地后吐到左边；漏接时球撞到 Doe 头上弹回左边、Doe 头上冒问号；
 * 两种情况球都滚进左下角的球堆；失误 3 次后显示结算，并把奖励和分数作为命令发给逻辑服务
 */

#include <stdio.h>
#include "pages.h"
#include "core/app_msg.h"
#include "core/games/tennis.h"
#include "hal/hal_audio.h"
#include "hal/hal_clock.h"
#include "ui/ui_records.h"
#include "ui/ui_theme.h"
#include "game_art.h"
#include "tennis_art.h"

#define FRAME_MS       15
#define MAX_STEP_MS    100  // 卡顿时一帧最多推进这么久
#define RESULT_LOCK_MS 800  // 结算出现后这么久内不接受「再来」，防止连按误触
#define RESULT_DELAY_MS 300 // 最后一球的动画播完后再停一下出结算
#define SCORE_FLASH_MS 400
#define BEEP_MS        400  // 预备开始时「滴」气泡显示多久
#define SHAKE_MS       300  // 发球前最后这段时间发球机抖动
#define RECOIL_MS      100  // 球弹出后发球机后坐
#define HOP_MS         120  // 叼住球落地后开心地蹦

// 发球机音效：预备时「滴」一声，球弹出时低沉的「嘭」
#define BEEP_HZ        1760
#define BEEP_TONE_MS   90
#define LAUNCH_HZ      220
#define LAUNCH_TONE_MS 60

#define FIELD_X        UI_SAFE_EDGE
#define FIELD_W        (240 - 2 * UI_SAFE_EDGE)
#define FIELD_BOTTOM   (240 - UI_SAFE_EDGE)
#define SKY_Y          30
#define HORIZON_Y      104
#define GROUND_H       (FIELD_BOTTOM - HORIZON_Y)
#define GROUND_BANDS   7
#define VANISH_X       52   // 地上的透视线汇聚到发球机附近的地平线

// 远处的发球机，炮口朝右
#define MACHINE_SCALE  2
#define MACHINE_X      34
#define MACHINE_BOTTOM 124
#define MACHINE_Y      (MACHINE_BOTTOM - TENNIS_MACHINE_H * MACHINE_SCALE)

// 近处的 Doe，朝左
#define DOE_SCALE      4
#define DOE_X          136
#define DOE_GROUND     206
#define DOE_Y          (DOE_GROUND - TENNIS_DOE_H * DOE_SCALE)
#define JUMP_PX        36   // 跳到最高时离地多高
#define MOUTH_DX       ((TENNIS_DOE_W - 1 - TENNIS_DOE_MOUTH_X) * DOE_SCALE + 2)
#define MOUTH_DY       (TENNIS_DOE_MOUTH_Y * DOE_SCALE + 2)

static const SpriteTurn MIRROR = {0, true};

// 球的轨迹：从炮口飞到 Doe 跳到最高时的嘴边。远处深度是 1000，到嘴边时是 BALL_NEAR，
// 屏幕上的大小和深度成反比，所以球越近越大，越近在屏幕上移动得越快
#define BALL_FROM_X    (MACHINE_X + TENNIS_MACHINE_W * MACHINE_SCALE + 2)
#define BALL_FROM_Y    (MACHINE_Y + TENNIS_MACHINE_NOZZLE_Y * MACHINE_SCALE + 1)
#define BALL_TO_X      (DOE_X + MOUTH_DX)
#define BALL_TO_Y      (DOE_Y - JUMP_PX + MOUTH_DY)
#define TRACK_FROM_Y   MACHINE_BOTTOM   // 球正下方的地面，用来放影子
#define TRACK_TO_Y     DOE_GROUND
#define BALL_NEAR      333
#define BALL_NEAR_ZOOM 768  // 到嘴边时放大 3 倍（256 是原大）
#define BALL_ARC       130  // 飞到一半时比直线高出多少，按近处的大小算
#define BALL_MAX_X     (FIELD_X + FIELD_W - 10)
#define TRAIL_N        2
#define TRAIL_GAP      60   // 拖尾每点落后多少千分比

#define PILE_MAX       8
#define PILE_FAR_Y     184  // 散落区最远处的地面，球放大 PILE_FAR_ZOOM；越往下越近、越大
#define PILE_FAR_ZOOM  512
#define PILE_NEAR_Y    226
#define PILE_NEAR_ZOOM 768

// 每球结算时球滚进球堆（要在 TENNIS_RESOLVE_MS 内滚完）
#define BONK_MS        340  // 漏接：撞到 Doe 头上弹回来，落地
#define BONK_ARC       22
#define RELEASE_MS     420  // 叼住：落地蹦完再把球吐出来
#define TOSS_MS        200
#define TOSS_ARC       14
#define SETTLE_END_MS  860

#define HEART_SCALE    2
#define HEART_GAP      4
#define DIGIT_SCALE    4

#define COLOR_SKY_TOP  0x1C2440
#define COLOR_SKY_LOW  0x4A6B9E
#define COLOR_GRASS_A  0x5A8F44
#define COLOR_GRASS_B  0x72AB56
#define COLOR_SHADOW   0x1A2A14

typedef struct {
    int32_t x, y;      // 球心
    int32_t ground_y;  // 球正下方的地面
    int32_t zoom;      // 256 是原大
} BallPos;

// 发过的球散落在左下角的草地上：球心横坐标、着地点、转几个 90°。按从远到近排，近的盖住远的
typedef struct {
    int16_t x, ground_y;
    uint8_t rot;
} PileSpot;

static const PileSpot PILE_SPOTS[PILE_MAX] = {
    {42, 188, 1}, {80, 193, 0}, {22, 201, 2}, {102, 207, 3},
    {57, 209, 0}, {34, 221, 3}, {76, 223, 1}, {112, 225, 2},
};

// 第 n 个落下的球放在哪个位置，打乱顺序看起来才像随手扔的
static const uint8_t PILE_ORDER[PILE_MAX] = {4, 2, 6, 0, 3, 5, 1, 7};

static TennisGame s_game;
static lv_timer_t* s_timer;
static uint32_t s_last_tick;
static uint32_t s_over_at;
static uint32_t s_result_at;
static uint32_t s_score_at;
static bool s_result_shown;
static uint16_t s_best;

// 界面上次画的状态，变了才重画
static TennisPhase s_shown_phase;
static uint16_t s_shown_score;
static uint8_t s_shown_misses;
static uint8_t s_shown_count;
static uint8_t s_shown_pile;
static const Sprite* s_shown_pose;
static const Sprite* s_shown_machine;

static lv_draw_buf_t* s_doe_buf;
static lv_draw_buf_t* s_machine_buf;
static lv_draw_buf_t* s_mountain_buf;
static lv_draw_buf_t* s_ball_buf;
static lv_draw_buf_t* s_heart_buf[TENNIS_MAX_MISSES];
static lv_draw_buf_t* s_digit_buf;

static lv_obj_t* s_score;
static lv_obj_t* s_best_label;
static lv_obj_t* s_hearts[TENNIS_MAX_MISSES];
static lv_obj_t* s_doe;
static lv_obj_t* s_machine;
static lv_obj_t* s_beep;
static lv_obj_t* s_ball;
static lv_obj_t* s_shadow;
static lv_obj_t* s_trail[TRAIL_N];
static lv_obj_t* s_pile[PILE_MAX];
static lv_obj_t* s_pile_shadow[PILE_MAX];
static lv_obj_t* s_feedback;
static lv_obj_t* s_huh;
static lv_obj_t* s_digit;
static lv_obj_t* s_go;
static lv_obj_t* s_intro;
static lv_obj_t* s_pause;
static lv_obj_t* s_result;
static lv_obj_t* s_result_desc;
static lv_obj_t* s_result_record;
static lv_obj_t* s_result_stars;
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

static lv_obj_t* rect(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    lv_obj_t* r = ui_box_create(parent);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, w, h);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    return r;
}

static lv_obj_t* dot(lv_obj_t* parent, uint32_t color, lv_opa_t opa) {
    lv_obj_t* d = rect(parent, 0, 0, 2, 2, color);
    lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(d, opa, 0);
    return d;
}

static void place_dot(lv_obj_t* d, int32_t cx, int32_t cy, int32_t size) {
    lv_obj_set_size(d, size, size);
    lv_obj_set_pos(d, cx - size / 2, cy - size / 2);
}

static BallPos ball_at(int32_t p) {
    BallPos b;
    if (p <= 1000) {
        int32_t depth = 1000 - (1000 - BALL_NEAR) * p / 1000;
        int32_t near = BALL_NEAR * 1000 / depth;                    // 相对到嘴边时的大小，千分比
        int32_t w = (near - BALL_NEAR) * 1000 / (1000 - BALL_NEAR);  // 屏幕上走了多远，千分比
        b.x = BALL_FROM_X + (BALL_TO_X - BALL_FROM_X) * w / 1000;
        b.ground_y = TRACK_FROM_Y + (TRACK_TO_Y - TRACK_FROM_Y) * w / 1000;
        b.y = BALL_FROM_Y + (BALL_TO_Y - BALL_FROM_Y) * w / 1000 -
              BALL_ARC * near / 1000 * 4 * p / 1000 * (1000 - p) / 1000;
        b.zoom = BALL_NEAR_ZOOM * near / 1000;
        return b;
    }
    // 没叼住：从 Doe 身边继续往镜头这边飞，落到地上
    int32_t q = p - 1000;
    b.x = BALL_TO_X + q * 150 / 1000;
    if (b.x > BALL_MAX_X) b.x = BALL_MAX_X;
    b.ground_y = TRACK_TO_Y + q * 16 / 1000;
    if (b.ground_y > FIELD_BOTTOM - 6) b.ground_y = FIELD_BOTTOM - 6;
    b.y = BALL_TO_Y - q * 50 / 1000 + q * q / 2500;
    int32_t floor = b.ground_y - TENNIS_BALL * BALL_NEAR_ZOOM / 256 / 2;
    if (b.y > floor) b.y = floor;
    b.zoom = BALL_NEAR_ZOOM;
    return b;
}

static bool caught(const TennisGame* g) {
    return g->phase != TENNIS_PHASE_COUNTDOWN && g->phase != TENNIS_PHASE_READY && g->phase != TENNIS_PHASE_FLIGHT &&
           (g->last == TENNIS_CATCH_PERFECT || g->last == TENNIS_CATCH_GOOD);
}

static bool missed(const TennisGame* g) {
    return (g->phase == TENNIS_PHASE_RESOLVE || g->phase == TENNIS_PHASE_OVER) && !caught(g);
}

static int32_t doe_y(const TennisGame* g) {
    int32_t y = DOE_Y - (int32_t)tennis_jump_lift(g) * JUMP_PX / 1000;
    // 叼住球落地后蹦两下
    if (caught(g) && !g->jumping && g->phase == TENNIS_PHASE_RESOLVE && (g->phase_ms / HOP_MS) % 2) y -= 3;
    return y;
}

static const Sprite* doe_pose(const TennisGame* g) {
    if (!g->jumping) return &tennis_doe_stand;
    if (g->jump_ms < TENNIS_JUMP_WINDUP_MS || g->jump_ms >= TENNIS_JUMP_AIR_END_MS) return &tennis_doe_crouch;
    if (tennis_mouth_open(g) && !caught(g)) return &tennis_doe_bite;
    return &tennis_doe_jump;
}

static void render_doe(const TennisGame* g) {
    const Sprite* pose = doe_pose(g);
    if (pose != s_shown_pose) {
        s_shown_pose = pose;
        sprite_draw_turned(s_doe, pose, DOE_SCALE, MIRROR);
    }
    int32_t y = doe_y(g);
    lv_obj_set_y(s_doe, y);

    bool huh = missed(g);
    lv_obj_set_hidden(s_huh, !huh);
    if (huh) lv_obj_set_pos(s_huh, DOE_X + 20, y - 24);
}

static void render_machine(const TennisGame* g) {
    bool ready = g->phase == TENNIS_PHASE_READY;
    uint32_t left = ready ? g->ready_ms - g->serve_ms : 0;
    // 预备时灯亮着，最后一段闪烁
    bool lamp = ready && (left > SHAKE_MS || (left / 60) % 2 == 0);
    const Sprite* sprite = lamp ? &tennis_machine_ready : &tennis_machine_idle;
    if (sprite != s_shown_machine) {
        s_shown_machine = sprite;
        sprite_draw_turned(s_machine, sprite, MACHINE_SCALE, MIRROR);
    }
    int32_t dx = 0;
    if (ready && left <= SHAKE_MS) dx = (left / 40) % 2 ? 1 : -1;
    if (g->phase == TENNIS_PHASE_FLIGHT && g->serve_ms - g->ready_ms < RECOIL_MS) dx = -2;
    lv_obj_set_x(s_machine, MACHINE_X + dx);
    lv_obj_set_hidden(s_beep, !(ready && g->serve_ms < BEEP_MS));
}

static int32_t pile_zoom(int32_t ground_y) {
    return PILE_FAR_ZOOM + (PILE_NEAR_ZOOM - PILE_FAR_ZOOM) * (ground_y - PILE_FAR_Y) / (PILE_NEAR_Y - PILE_FAR_Y);
}

static int32_t ball_size(int32_t zoom) {
    return TENNIS_BALL * zoom / 256;
}

// 球停在地上时的球心高度
static int32_t resting_y(int32_t ground_y, int32_t zoom) {
    return ground_y - ball_size(zoom) / 2 + 1;
}

// 第 index 个发出的球最后滚到哪个位置；位置用完了就滚进最后一个位置
static uint8_t pile_spot(uint16_t index) {
    return PILE_ORDER[index < PILE_MAX ? index : PILE_MAX - 1];
}

// 离终点还要滚 dist 像素时转了多少：每滚四分之一圈转 90°，滚到终点正好是这个位置的朝向
static int32_t roll_rotation(uint8_t spot, int32_t dist, int32_t zoom) {
    int32_t quarter_turns = dist * 400 / (314 * ball_size(zoom));
    return (int32_t)((PILE_SPOTS[spot].rot + quarter_turns) % 4) * 900;
}

static int32_t lerp(int32_t a, int32_t b, int32_t permille) {
    return a + (b - a) * permille / 1000;
}

// 球从 from 抛起，落到去往 spot 的半路上，弹一下再滚过去停住；t 是抛出后过了多久，滚到 total_ms 停
static BallPos settle(BallPos from, uint8_t spot, uint32_t t, uint32_t fly_ms, uint32_t total_ms, int32_t arc,
                      int32_t* rotation) {
    const PileSpot* to = &PILE_SPOTS[spot];
    int32_t to_zoom = pile_zoom(to->ground_y);
    const int32_t land = 550;  // 落地点在全程的千分比
    int32_t land_x = lerp(from.x, to->x, land);
    int32_t land_g = lerp(from.ground_y, to->ground_y, land);
    int32_t land_zoom = lerp(from.zoom, to_zoom, land);
    BallPos b;
    if (t < fly_ms) {
        int32_t f = (int32_t)(t * 1000 / fly_ms);
        b.x = lerp(from.x, land_x, f);
        b.ground_y = lerp(from.ground_y, land_g, f);
        b.zoom = lerp(from.zoom, land_zoom, f);
        b.y = lerp(from.y, resting_y(land_g, land_zoom), f) - arc * 4 * f / 1000 * (1000 - f) / 1000;
        *rotation = roll_rotation(spot, to->x > land_x ? to->x - land_x : land_x - to->x, land_zoom);
        return b;
    }
    int32_t f = t >= total_ms ? 1000 : (int32_t)((t - fly_ms) * 1000 / (total_ms - fly_ms));
    int32_t eased = 1000 - (1000 - f) * (1000 - f) / 1000;  // 越滚越慢
    b.x = lerp(land_x, to->x, eased);
    b.ground_y = lerp(land_g, to->ground_y, eased);
    b.zoom = lerp(land_zoom, to_zoom, eased);
    b.y = resting_y(b.ground_y, b.zoom);
    if (f < 300) b.y -= 5 * 4 * (f * 1000 / 300) / 1000 * (1000 - f * 1000 / 300) / 1000;  // 落地弹一下
    *rotation = roll_rotation(spot, to->x > b.x ? to->x - b.x : b.x - to->x, b.zoom);
    return b;
}

// 结算阶段球去哪：漏接的球撞到 Doe 头上弹回左边，叼住的球等 Doe 落地后吐到左边，都滚进球堆
static BallPos resolve_ball(const TennisGame* g, int32_t* rotation, bool* in_mouth) {
    uint8_t spot = pile_spot(g->serves - 1);
    *in_mouth = false;
    if (caught(g)) {
        if (g->phase_ms < RELEASE_MS) {
            *in_mouth = true;
            *rotation = 0;
            return (BallPos){DOE_X + MOUTH_DX, doe_y(g) + MOUTH_DY, DOE_GROUND, BALL_NEAR_ZOOM};
        }
        BallPos mouth = {DOE_X + MOUTH_DX, DOE_Y + MOUTH_DY, DOE_GROUND, BALL_NEAR_ZOOM};
        return settle(mouth, spot, g->phase_ms - RELEASE_MS, TOSS_MS, SETTLE_END_MS - RELEASE_MS, TOSS_ARC, rotation);
    }
    // 漏接判定那一刻球飞到的位置
    int32_t p = (int32_t)((g->serve_ms - g->phase_ms - g->ready_ms) * 1000 / g->arrive_ms);
    return settle(ball_at(p), spot, g->phase_ms, BONK_MS, SETTLE_END_MS, BONK_ARC, rotation);
}

static void render_ball(const TennisGame* g) {
    int32_t p = tennis_ball_progress(g);
    bool flight = g->phase == TENNIS_PHASE_FLIGHT;
    bool resolving = g->phase == TENNIS_PHASE_RESOLVE;
    bool shown = (flight && p >= 0) || resolving;
    bool in_mouth = false;
    int32_t rotation = 0;
    BallPos b = resolving ? resolve_ball(g, &rotation, &in_mouth) : ball_at(p < 0 ? 0 : p);
    lv_obj_set_hidden(s_ball, !shown);
    lv_obj_set_hidden(s_shadow, !shown || in_mouth);

    lv_obj_set_pos(s_ball, b.x - TENNIS_BALL / 2, b.y - TENNIS_BALL / 2);
    lv_image_set_scale(s_ball, (uint32_t)b.zoom);
    lv_image_set_rotation(s_ball, rotation);

    int32_t shadow_w = 4 + 14 * b.zoom / BALL_NEAR_ZOOM;
    lv_obj_set_size(s_shadow, shadow_w, 2 + shadow_w / 6);
    lv_obj_set_pos(s_shadow, b.x - shadow_w / 2, b.ground_y - 1);

    for (int i = 0; i < TRAIL_N; i++) {
        int32_t tp = p - TRAIL_GAP * (i + 1);
        bool on = flight && tp >= 0;
        lv_obj_set_hidden(s_trail[i], !on);
        if (!on) continue;
        BallPos t = ball_at(tp);
        place_dot(s_trail[i], t.x, t.y, ball_size(t.zoom) * (3 - i) / 4);
    }
}

static void render_pile(const TennisGame* g) {
    // 已经滚到位的球算进球堆，正在发的这一球不算
    uint16_t done = g->phase == TENNIS_PHASE_OVER ? g->serves : (g->serves ? g->serves - 1 : 0);
    uint8_t n = done > PILE_MAX ? PILE_MAX : (uint8_t)done;
    if (n == s_shown_pile) return;
    s_shown_pile = n;
    for (uint8_t i = 0; i < PILE_MAX; i++) {
        uint8_t spot = PILE_ORDER[i];
        lv_obj_set_hidden(s_pile[spot], i >= n);
        lv_obj_set_hidden(s_pile_shadow[spot], i >= n);
    }
}

static const char* feedback_text(const TennisGame* g, uint32_t* color) {
    switch (g->last) {
        case TENNIS_CATCH_PERFECT: *color = UI_COLOR_GLOW; return "Perfect!";
        case TENNIS_CATCH_GOOD:    *color = UI_COLOR_FOCUS; return "Nice!";
        case TENNIS_MISS_EARLY:    *color = UI_COLOR_ALERT; return "跳早了";
        default:                   *color = UI_COLOR_ALERT; return g->jump_at >= 0 ? "跳晚了" : "漏接了";
    }
}

static void render_hud(const TennisGame* g) {
    uint32_t now = lv_tick_get();
    if (g->score != s_shown_score) {
        if (g->score > s_shown_score) s_score_at = now;
        s_shown_score = g->score;
        lv_label_set_text_fmt(s_score, "得分 %u", (unsigned)g->score);
    }
    bool flash = lv_tick_diff(now, s_score_at) < SCORE_FLASH_MS;
    lv_obj_set_style_text_color(s_score, lv_color_hex(flash ? UI_COLOR_GLOW : UI_COLOR_TEXT), 0);

    if (g->misses != s_shown_misses) {
        s_shown_misses = g->misses;
        for (uint8_t i = 0; i < TENNIS_MAX_MISSES; i++) {
            bool full = i < TENNIS_MAX_MISSES - g->misses;
            sprite_draw(s_hearts[i], full ? &tennis_heart_full : &tennis_heart_empty, HEART_SCALE);
        }
    }
}

static void render_countdown(const TennisGame* g) {
    uint8_t n = g->phase == TENNIS_PHASE_COUNTDOWN ? tennis_countdown(g) : 0xFF;
    if (n == s_shown_count) return;
    s_shown_count = n;
    lv_obj_set_hidden(s_digit, n == 0 || n == 0xFF);
    lv_obj_set_hidden(s_go, n != 0);
    lv_obj_set_hidden(s_intro, n == 0xFF);
    if (n && n != 0xFF) sprite_draw(s_digit, &game_digits[n - 1], DIGIT_SCALE);
}

static void enter_phase(const TennisGame* g, uint32_t now) {
    lv_obj_set_hidden(s_feedback, g->phase != TENNIS_PHASE_RESOLVE);
    if (g->phase == TENNIS_PHASE_RESOLVE) {
        uint32_t color;
        lv_label_set_text(s_feedback, feedback_text(g, &color));
        lv_obj_set_style_text_color(s_feedback, lv_color_hex(color), 0);
    }
    if (g->phase == TENNIS_PHASE_READY) hal_audio_tone(BEEP_HZ, BEEP_TONE_MS);
    if (g->phase == TENNIS_PHASE_FLIGHT) hal_audio_tone(LAUNCH_HZ, LAUNCH_TONE_MS);
    if (g->phase == TENNIS_PHASE_OVER) s_over_at = now;
}

static void render(void) {
    const TennisGame* g = &s_game;
    if (g->phase != s_shown_phase) {
        s_shown_phase = g->phase;
        enter_phase(g, lv_tick_get());
    }
    render_countdown(g);
    render_hud(g);
    render_machine(g);
    render_pile(g);
    render_doe(g);
    render_ball(g);
    lv_obj_set_hidden(s_pause, !g->paused);
}

static void show_result(void) {
    const TennisGame* g = &s_game;
    TennisReward reward = tennis_reward(g->score);
    AppCmd cmd = {
        .type = APP_CMD_GAME_RESULT,
        .result = {.game = GAME_TENNIS, .stars = reward.stars, .shells = reward.shells, .spirit = reward.spirit,
                   .score = g->score},
    };
    app_cmd_send(&cmd);

    bool record = g->score > s_best;
    lv_label_set_text_fmt(s_result_desc, "叼住 %u 个球，完美 %u 次", (unsigned)g->score, (unsigned)g->perfects);
    if (record) {
        lv_label_set_text(s_result_record, "新纪录!");
        lv_obj_set_style_text_color(s_result_record, lv_color_hex(UI_COLOR_GLOW), 0);
        s_best = g->score;
        lv_label_set_text_fmt(s_best_label, "最高 %02u", (unsigned)s_best);
    } else {
        lv_label_set_text_fmt(s_result_record, "最高纪录 %u", (unsigned)s_best);
        lv_obj_set_style_text_color(s_result_record, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    }
    lv_label_set_text_fmt(s_result_stars, "%s%s%s", reward.stars >= 1 ? "★" : "☆",
                          reward.stars >= 2 ? "★" : "☆", reward.stars >= 3 ? "★" : "☆");
    lv_label_set_text_fmt(s_result_shells, "贝壳 +%u", (unsigned)reward.shells);
    lv_obj_set_hidden(s_result, false);

    s_result_shown = true;
    s_result_at = lv_tick_get();
    printf("[Tennis] 叼住 %u 个（完美 %u），发了 %u 球，%u 星，用时 %.1f 秒%s\n", (unsigned)g->score,
           (unsigned)g->perfects, (unsigned)g->serves, (unsigned)reward.stars, g->elapsed_ms / 1000.0,
           record ? "，新纪录" : "");
}

static void advance(void) {
    uint32_t now = lv_tick_get();
    uint32_t dt = now - s_last_tick;
    s_last_tick = now;
    tennis_step(&s_game, dt > MAX_STEP_MS ? MAX_STEP_MS : dt);
}

static void on_frame(lv_timer_t* timer) {
    (void)timer;
    advance();
    render();
    if (s_game.phase == TENNIS_PHASE_OVER && !s_result_shown && lv_tick_elaps(s_over_at) >= RESULT_DELAY_MS) {
        show_result();
    }
}

static void start(void) {
    tennis_init(&s_game, (uint32_t)hal_clock_us());
    s_last_tick = lv_tick_get();
    s_result_shown = false;
    s_shown_phase = TENNIS_PHASE_OVER;
    s_shown_score = 0xFFFF;
    s_shown_misses = 0xFF;
    s_shown_count = 0xFE;
    s_shown_pile = 0xFF;
    s_shown_pose = NULL;
    s_shown_machine = NULL;
    s_score_at = s_last_tick - SCORE_FLASH_MS;
    s_best = ui_records_best(GAME_TENNIS);
    lv_label_set_text_fmt(s_best_label, "最高 %02u", (unsigned)s_best);
    lv_obj_set_hidden(s_result, true);
    render();
}

static void create_sky(lv_obj_t* scr) {
    lv_obj_t* sky = rect(scr, FIELD_X, SKY_Y, FIELD_W, HORIZON_Y - SKY_Y, COLOR_SKY_TOP);
    lv_obj_set_style_bg_grad_color(sky, lv_color_hex(COLOR_SKY_LOW), 0);
    lv_obj_set_style_bg_grad_dir(sky, LV_GRAD_DIR_VER, 0);

    static const lv_point_t STARS[] = {{30, 40}, {74, 52}, {118, 36}, {160, 58}, {204, 44}, {96, 70}, {190, 72}};
    for (size_t i = 0; i < sizeof(STARS) / sizeof(STARS[0]); i++) {
        lv_obj_t* s = rect(scr, STARS[i].x, STARS[i].y, 2, 2, UI_COLOR_CREAM);
        lv_obj_set_style_bg_opa(s, i % 2 ? LV_OPA_50 : LV_OPA_80, 0);
    }

    lv_obj_t* mountains = sprite_canvas_create(scr, &s_mountain_buf, TENNIS_MOUNTAINS_W * 3, TENNIS_MOUNTAINS_H * 3);
    sprite_draw(mountains, &tennis_mountains, 3);
    lv_obj_set_pos(mountains, FIELD_X + FIELD_W - TENNIS_MOUNTAINS_W * 3, HORIZON_Y - TENNIS_MOUNTAINS_H * 3);
}

// 草地：越近的横条越宽，几条透视线从发球机那边的地平线放射出来
static void create_ground(lv_obj_t* scr) {
    lv_obj_t* ground = ui_box_create(scr);
    lv_obj_set_pos(ground, FIELD_X, HORIZON_Y);
    lv_obj_set_size(ground, FIELD_W, GROUND_H);
    for (int32_t k = 0; k < GROUND_BANDS; k++) {
        int32_t top = GROUND_H * k * k / (GROUND_BANDS * GROUND_BANDS);
        int32_t bottom = GROUND_H * (k + 1) * (k + 1) / (GROUND_BANDS * GROUND_BANDS);
        rect(ground, 0, top, FIELD_W, bottom - top, k % 2 ? COLOR_GRASS_B : COLOR_GRASS_A);
    }

    static const int32_t RAY_BOTTOM_X[] = {-160, -20, 120, 260, 400};
    static lv_point_precise_t rays[sizeof(RAY_BOTTOM_X) / sizeof(RAY_BOTTOM_X[0])][2];
    for (size_t i = 0; i < sizeof(RAY_BOTTOM_X) / sizeof(RAY_BOTTOM_X[0]); i++) {
        rays[i][0] = (lv_point_precise_t){VANISH_X - FIELD_X, 0};
        rays[i][1] = (lv_point_precise_t){RAY_BOTTOM_X[i] - FIELD_X, GROUND_H};
        lv_obj_t* line = lv_line_create(ground);
        lv_line_set_points(line, rays[i], 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(UI_COLOR_CREAM), 0);
        lv_obj_set_style_line_opa(line, LV_OPA_30, 0);
    }
}

static void create_scene(lv_obj_t* scr) {
    create_sky(scr);
    create_ground(scr);

    s_machine = sprite_canvas_create(scr, &s_machine_buf, TENNIS_MACHINE_W * MACHINE_SCALE,
                                     TENNIS_MACHINE_H * MACHINE_SCALE);
    lv_obj_set_pos(s_machine, MACHINE_X, MACHINE_Y);

    // 「滴!」气泡
    s_beep = ui_box_create(scr);
    lv_obj_set_size(s_beep, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(s_beep, lv_color_hex(UI_COLOR_CREAM), 0);
    lv_obj_set_style_bg_opa(s_beep, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_beep, 4, 0);
    lv_obj_set_style_pad_hor(s_beep, 3, 0);
    lv_obj_set_style_pad_ver(s_beep, 1, 0);
    lv_obj_t* beep = label(s_beep, &font_ui_12, UI_COLOR_INK);
    lv_label_set_text(beep, "滴!");
    lv_obj_set_pos(s_beep, MACHINE_X + 24, MACHINE_Y - 18);

    s_shadow = dot(scr, COLOR_SHADOW, LV_OPA_50);

    sprite_buf_render(&s_ball_buf, &tennis_ball, 1, (SpriteTurn){0, false});
    for (int i = 0; i < PILE_MAX; i++) {
        const PileSpot* at = &PILE_SPOTS[i];
        int32_t zoom = pile_zoom(at->ground_y);
        int32_t size = TENNIS_BALL * zoom / 256;
        s_pile_shadow[i] = dot(scr, COLOR_SHADOW, LV_OPA_40);
        lv_obj_set_size(s_pile_shadow[i], size, 3 + size / 8);
        lv_obj_set_pos(s_pile_shadow[i], at->x - size / 2 + 2, at->ground_y - 2);
        s_pile[i] = lv_image_create(scr);
        lv_image_set_src(s_pile[i], s_ball_buf);
        lv_image_set_scale(s_pile[i], (uint32_t)zoom);
        lv_image_set_rotation(s_pile[i], at->rot * 900);
        lv_image_set_antialias(s_pile[i], false);
        lv_obj_set_pos(s_pile[i], at->x - TENNIS_BALL / 2, resting_y(at->ground_y, zoom) - TENNIS_BALL / 2);
    }

    s_doe = sprite_canvas_create(scr, &s_doe_buf, TENNIS_DOE_W * DOE_SCALE, TENNIS_DOE_H * DOE_SCALE);
    lv_obj_set_pos(s_doe, DOE_X, DOE_Y);
    s_huh = label(scr, &font_ui_24, UI_COLOR_FOCUS);
    lv_label_set_text(s_huh, "?");

    for (int i = 0; i < TRAIL_N; i++) s_trail[i] = dot(scr, 0xD4F04C, i ? LV_OPA_30 : LV_OPA_50);
    s_ball = lv_image_create(scr);
    lv_image_set_src(s_ball, s_ball_buf);
    lv_image_set_antialias(s_ball, false);

    s_feedback = label(scr, &font_ui_24, UI_COLOR_GLOW);
    lv_obj_align(s_feedback, LV_ALIGN_TOP_MID, 0, SKY_Y + 8);
}

static void create_header(lv_obj_t* scr) {
    s_score = label(scr, &font_ui_12, UI_COLOR_TEXT);
    lv_obj_set_pos(s_score, FIELD_X + 2, 11);
    int32_t heart_w = TENNIS_HEART_W * HEART_SCALE;
    int32_t hearts_x = (240 - TENNIS_MAX_MISSES * heart_w - (TENNIS_MAX_MISSES - 1) * HEART_GAP) / 2;
    for (uint8_t i = 0; i < TENNIS_MAX_MISSES; i++) {
        s_hearts[i] = sprite_canvas_create(scr, &s_heart_buf[i], heart_w, TENNIS_HEART_H * HEART_SCALE);
        lv_obj_set_pos(s_hearts[i], hearts_x + i * (heart_w + HEART_GAP), 12);
    }
    s_best_label = label(scr, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_obj_align(s_best_label, LV_ALIGN_TOP_RIGHT, -FIELD_X - 2, 11);
}

static void create_overlays(lv_obj_t* scr) {
    // 倒计时时说明盖住天空，露出地平线上的发球机
    s_intro = column_panel(scr, 204, 58);
    lv_obj_align(s_intro, LV_ALIGN_TOP_MID, 0, SKY_Y + 2);
    lv_obj_set_style_pad_row(s_intro, 2, 0);
    lv_obj_t* line = label(s_intro, &font_ui_24, UI_COLOR_FOCUS);
    lv_label_set_text(line, "接网球");
    line = label(s_intro, &font_ui_12, UI_COLOR_TEXT);
    lv_label_set_text(line, "看准时机按确定，跳起叼住网球");
    line = label(s_intro, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "选择:暂停  返回:退出");

    s_digit = sprite_canvas_create(scr, &s_digit_buf, GAME_DIGIT_W * DIGIT_SCALE, GAME_DIGIT_H * DIGIT_SCALE);
    lv_obj_align(s_digit, LV_ALIGN_TOP_MID, -16, 132);
    s_go = label(scr, &font_ui_24, UI_COLOR_GLOW);
    lv_label_set_text(s_go, "开始!");
    lv_obj_align(s_go, LV_ALIGN_TOP_MID, -16, 138);

    s_pause = column_panel(scr, 160, 64);
    lv_obj_align(s_pause, LV_ALIGN_CENTER, 0, 10);
    line = label(s_pause, &font_ui_24, UI_COLOR_FOCUS);
    lv_label_set_text(line, "暂停");
    line = label(s_pause, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "选择:继续  返回:退出");
    lv_obj_set_hidden(s_pause, true);

    s_result = column_panel(scr, 200, 150);
    lv_obj_align(s_result, LV_ALIGN_CENTER, 0, 10);
    line = label(s_result, &font_ui_24, UI_COLOR_GLOW);
    lv_label_set_text(line, "训练结束");
    s_result_desc = label(s_result, &font_ui_12, UI_COLOR_TEXT);
    s_result_record = label(s_result, &font_ui_12, UI_COLOR_TEXT_DIM);
    s_result_stars = label(s_result, &font_ui_24, UI_COLOR_FOCUS);
    s_result_shells = label(s_result, &font_ui_12, UI_COLOR_TEXT);
    line = label(s_result, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(line, "确定:再来  返回:菜单");
}

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    ui_screen_dark(scr);
    create_header(scr);
    create_scene(scr);
    create_overlays(scr);

    start();
    s_timer = lv_timer_create(on_frame, FRAME_MS, NULL);
}

static void destroy(void) {
    if (s_game.phase != TENNIS_PHASE_OVER) printf("[Tennis] 中途退出，没有奖励\n");
    if (s_timer) lv_timer_delete(s_timer);
    s_timer = NULL;
}

static bool on_input(const InputEvent* e) {
    if (e->key == INPUT_KEY_BACK) return false;
    if (s_game.phase == TENNIS_PHASE_OVER) {
        if (s_result_shown && e->type == INPUT_EVT_SHORT_PRESS && e->key == INPUT_KEY_OK &&
            lv_tick_elaps(s_result_at) >= RESULT_LOCK_MS) {
            start();
        }
        return true;
    }
    if (e->type != INPUT_EVT_PRESS) return true;
    // 按键在两帧之间到达，先把规则推进到现在，起跳时刻才准
    advance();
    if (e->key == INPUT_KEY_OK) tennis_jump(&s_game);
    if (e->key == INPUT_KEY_SELECT) tennis_toggle_pause(&s_game);
    render();
    return true;
}

const Page page_tennis = {
    .name = "tennis",
    .create = create,
    .destroy = destroy,
    .on_input = on_input,
    .idle_timeout_ms = 0,
};
