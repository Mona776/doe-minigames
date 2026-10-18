/**
 * Doe 拔河 - 开局倒计时 3、2、1（界面全部隐藏，只留场景）；每轮主人先拉（轻、中、重三种动画），
 * 播完才出力量条：红针是主人本轮拉力，黄针自动摆动，确定键锁定力量（按下即生效，按住不重复）；
 * 黄针越接近红针越好，返回键中途退出、没有奖励
 * 规则在 core/games/tug.c；这里每帧推进规则，画位置格、力量条、Doe、主人手臂、绳子和玩具
 * 一局结束后显示结算，并把奖励作为命令发给逻辑服务
 */

#include <stdio.h>
#include "pages.h"
#include "core/app_msg.h"
#include "core/games/tug.h"
#include "hal/hal_clock.h"
#include "ui/ui_theme.h"
#include "game_art.h"
#include "tug_art.h"

#define FRAME_MS       15
#define MAX_STEP_MS    100  // 卡顿时一帧最多推进这么久
#define RESULT_LOCK_MS 800  // 结算出现后这么久内不接受「再来」，防止连按误触

// 顶部的位置条：一根绳子，两端是 DOE 和主人的终点，小玩具停在当前标记上
#define TRACK_STEP     24
#define TRACK_X0       ((240 - TRACK_STEP * (TUG_CELLS - 1)) / 2)
#define TRACK_Y        54   // 绳子中线
#define TRACK_TOY_SCALE 2

#define METER_X        20
#define METER_Y        106
#define METER_W        200
#define METER_H        14
#define METER_INNER_W  (METER_W - 4)
#define POINTER_W      6
#define POINTER_H      24

#define DIGIT_SCALE    5
#define DOE_SCALE      3
#define TOY_SCALE      3
#define GROUND_Y       188
#define DOE_X          22
#define DOE_TOP        (GROUND_Y - TUG_DOE_H * DOE_SCALE)
#define ROPE_Y         (DOE_TOP + TUG_DOE_MOUTH_Y * DOE_SCALE + 1)
#define MOUTH_X        (DOE_X + TUG_DOE_MOUTH_X * DOE_SCALE)
#define HAND_X         180
#define HAND_W         20
#define HAND_H         18
#define SLEEVE_H       16
#define TOY_X0         (MOUTH_X + 12)  // 玩具在第 0 格时的中心
#define TOY_STEP       ((HAND_X - 8 - TOY_X0) / (TUG_CELLS - 1))
#define TOY_SPEED      2               // 玩具滑到新格子时每帧移动的像素
#define SLIP_SLIDE     14              // 滑倒时 Doe 往前滑的距离

// 主人发力动画的节奏，按发力动画进度的千分比：先往前送一点蓄力，再猛地拽回，之后保持
#define WINDUP_END     250
#define YANK_END       450
#define WINDUP_DX      4

#define COLOR_SKIN     0xF2C29B
#define COLOR_SLEEVE   0x5B7BD5
#define COLOR_ROPE     0xD9B26F

// 三种力度的表现：手往回拽多少、Doe 被拖过去多少、要不要抖
typedef struct {
    int32_t pull;
    int32_t drag;
    bool shake;
    const char* text;
    uint32_t color;
} ForceFx;

static const ForceFx FORCE_FX[] = {
    [TUG_FORCE_LIGHT]  = {6, 1, false, "主人轻轻一拉", UI_COLOR_TEXT_DIM},
    [TUG_FORCE_MID]    = {12, 4, false, "主人用力一拉", UI_COLOR_TEXT},
    [TUG_FORCE_STRONG] = {20, 8, true, "主人猛地一拽!", UI_COLOR_ALERT},
};

static TugGame s_game;
static lv_timer_t* s_timer;
static uint32_t s_last_tick;
static uint32_t s_result_at;
static bool s_result_shown;
static uint16_t s_shown_round;
static TugPhase s_shown_phase;
static uint8_t s_shown_count;
static int32_t s_toy_x;
static int32_t s_track_toy_x;

static lv_draw_buf_t* s_doe_buf;
static lv_draw_buf_t* s_toy_buf;
static lv_draw_buf_t* s_track_toy_buf;
static lv_draw_buf_t* s_digit_buf;
static const Sprite* s_doe_sprite;

static lv_obj_t* s_hud;    // 顶部信息、提示文字、底部提示栏，倒计时时整组隐藏
static lv_obj_t* s_meter;  // 力量条，主人发力动画播完才出现
static lv_obj_t* s_round;
static lv_obj_t* s_track_toy;
static lv_obj_t* s_match_band;
static lv_obj_t* s_perfect_band;
static lv_obj_t* s_pointer;
static lv_obj_t* s_owner_pointer;
static lv_obj_t* s_feedback;
static lv_obj_t* s_digit;
static lv_obj_t* s_go;
static lv_obj_t* s_rope;
static lv_point_precise_t s_rope_points[3];
static lv_obj_t* s_doe;
static lv_obj_t* s_sleeve;
static lv_obj_t* s_hand;
static lv_obj_t* s_toy;
static lv_obj_t* s_hint_label;
static lv_obj_t* s_result;
static lv_obj_t* s_result_title;
static lv_obj_t* s_result_desc;
static lv_obj_t* s_result_stars;
static lv_obj_t* s_result_shells;

static lv_obj_t* rect(lv_obj_t* parent, uint32_t color) {
    lv_obj_t* r = ui_box_create(parent);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    return r;
}

static lv_obj_t* outlined(lv_obj_t* parent, uint32_t color) {
    lv_obj_t* r = rect(parent, color);
    lv_obj_set_style_border_width(r, 2, 0);
    lv_obj_set_style_border_color(r, lv_color_hex(UI_COLOR_INK), 0);
    lv_obj_set_style_radius(r, 3, 0);
    return r;
}

static lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

// 铺满屏幕的透明层，用来整组显示/隐藏；子控件坐标仍按屏幕算
static lv_obj_t* layer(lv_obj_t* parent) {
    lv_obj_t* l = ui_box_create(parent);
    lv_obj_set_size(l, 240, 240);
    lv_obj_set_pos(l, 0, 0);
    return l;
}

static int32_t power_x(int32_t power) {
    return METER_X + 2 + power * METER_INNER_W / TUG_POWER_MAX;
}

static int32_t meter_x(uint8_t power) {
    return power_x(power) - POINTER_W / 2;
}

static void place_band(lv_obj_t* band, uint8_t center, int32_t range) {
    int32_t from = power_x(LV_MAX(center - range, 0));
    int32_t to = power_x(LV_MIN(center + range, TUG_POWER_MAX));
    lv_obj_set_pos(band, from, METER_Y + 2);
    lv_obj_set_size(band, LV_MAX(to - from, 1), METER_H - 4);
}

static int32_t toy_target_x(uint8_t cell) {
    return TOY_X0 + cell * TOY_STEP;
}

static int32_t track_x(uint8_t cell) {
    return TRACK_X0 + cell * TRACK_STEP;
}

static void set_feedback(const char* text, uint32_t color) {
    lv_label_set_text(s_feedback, text);
    lv_obj_set_style_text_color(s_feedback, lv_color_hex(color), 0);
    lv_obj_set_hidden(s_feedback, false);
}

static void show_round_result(const TugGame* g) {
    switch (g->pull) {
        case TUG_PULL_PERFECT:
            set_feedback("完美反拉!", UI_COLOR_GLOW);
            break;
        case TUG_PULL_GOOD:
            set_feedback("反拉成功", UI_COLOR_GLOW);
            break;
        case TUG_PULL_WEAK:
            set_feedback(g->lost_cells > 1 ? "力量不足，被拉走两格" : "力量不足，拉不动", UI_COLOR_ALERT);
            break;
        case TUG_PULL_SLIP:
            set_feedback(g->lost_cells > 1 ? "用力过猛，滑出两格!" : "用力过猛，滑倒了!", UI_COLOR_ALERT);
            break;
    }
}

static void enter_phase(const TugGame* g) {
    bool counting = g->phase == TUG_PHASE_COUNTDOWN;
    lv_obj_set_hidden(s_hud, counting);
    lv_obj_set_hidden(s_meter, counting || g->phase == TUG_PHASE_OWNER);
    if (!counting) {
        lv_obj_set_hidden(s_digit, true);
        lv_obj_set_hidden(s_go, true);
    }

    switch (g->phase) {
        case TUG_PHASE_OWNER:
            set_feedback(FORCE_FX[g->force].text, FORCE_FX[g->force].color);
            break;
        case TUG_PHASE_SWING:
            if (g->round == 1) {
                set_feedback("黄针对准红针时按确定", UI_COLOR_TEXT_DIM);
            } else if (g->force == TUG_FORCE_STRONG) {
                char text[48];
                lv_snprintf(text, sizeof(text), "主人拉力 %u  没接住退两格", (unsigned)g->owner_power);
                set_feedback(text, UI_COLOR_ALERT);
            } else {
                char text[32];
                lv_snprintf(text, sizeof(text), "主人拉力 %u", (unsigned)g->owner_power);
                set_feedback(text, UI_COLOR_TEXT_DIM);
            }
            break;
        case TUG_PHASE_RESOLVE:
            show_round_result(g);
            break;
        default:
            lv_obj_set_hidden(s_feedback, true);
            break;
    }
}

static void render_countdown(const TugGame* g) {
    uint8_t n = tug_countdown(g);
    if (n == s_shown_count) return;
    s_shown_count = n;
    lv_obj_set_hidden(s_digit, n == 0);
    lv_obj_set_hidden(s_go, n != 0);
    if (n) sprite_draw(s_digit, &game_digits[n - 1], DIGIT_SCALE);
}

static void render_hud(const TugGame* g) {
    if (g->phase != s_shown_phase) {
        s_shown_phase = g->phase;
        enter_phase(g);
    }
    if (g->phase == TUG_PHASE_COUNTDOWN) {
        render_countdown(g);
        return;
    }
    if (g->round != s_shown_round) {
        s_shown_round = g->round;
        lv_label_set_text_fmt(s_round, "第 %u 轮", (unsigned)g->round);
        place_band(s_match_band, g->owner_power, g->match_range);
        place_band(s_perfect_band, g->owner_power, g->perfect_range);
        lv_obj_set_x(s_owner_pointer, meter_x(g->owner_power));
    }
    int32_t target = track_x(g->cell);
    if (s_track_toy_x < target) s_track_toy_x = LV_MIN(s_track_toy_x + TOY_SPEED, target);
    if (s_track_toy_x > target) s_track_toy_x = LV_MAX(s_track_toy_x - TOY_SPEED, target);
    lv_obj_set_x(s_track_toy, s_track_toy_x - TUG_TOY_SIZE * TRACK_TOY_SCALE / 2);
    lv_obj_set_x(s_pointer, meter_x(g->phase == TUG_PHASE_SWING ? tug_meter(g) : g->doe_power));
}

// 主人这一拉现在拽到哪：返回手往回拽的距离（负数是往 Doe 那边送），drag 返回 Doe 被拖过去多少
static int32_t owner_pull(const TugGame* g, uint32_t t, int32_t* drag) {
    const ForceFx* fx = &FORCE_FX[g->force];
    int32_t pull = fx->pull;
    int32_t shake = fx->shake && (t / 40) % 2 ? 2 : 0;
    *drag = fx->drag;
    if (g->phase == TUG_PHASE_OWNER) {
        uint32_t p = g->phase_ms * 1000 / TUG_OWNER_ANIM_MS;
        if (p < WINDUP_END) {
            *drag = 0;
            return -WINDUP_DX * (int32_t)p / WINDUP_END;
        }
        if (p < YANK_END) {
            int32_t k = (int32_t)(p - WINDUP_END);
            int32_t span = YANK_END - WINDUP_END;
            *drag = fx->drag * k / span;
            return -WINDUP_DX + (pull + WINDUP_DX) * k / span;
        }
        *drag += shake;
        return pull + shake;
    }
    // 力量条摆动时主人一直绷着，重力度会轻微发抖
    int32_t tremble = fx->shake && (t / 80) % 2 ? 1 : 0;
    return pull + tremble;
}

static void render(void) {
    const TugGame* g = &s_game;
    uint32_t t = lv_tick_get();
    render_hud(g);

    bool resolving = g->phase == TUG_PHASE_RESOLVE;
    bool tense = g->phase == TUG_PHASE_OWNER || g->phase == TUG_PHASE_SWING;
    bool lost_pull = resolving || g->result == TUG_RESULT_LOSE;
    const Sprite* doe = &tug_doe_stand;
    int32_t dx = 0, dy = 0, mouth_dy = 0;
    int32_t hand_dx = 0, hand_dy = 0;
    int32_t tension = 0;  // 绳子被主人绷紧时，玩具往主人那边偏多少
    if (g->result == TUG_RESULT_WIN) {
        dy = (t / 150) % 2 ? -6 : 0;
        hand_dx = 10;
        hand_dy = 12;
    } else if (resolving && (g->pull == TUG_PULL_PERFECT || g->pull == TUG_PULL_GOOD)) {
        doe = &tug_doe_pull;
        dx = g->pull == TUG_PULL_PERFECT ? -8 : -5;
        dy = -2;
        hand_dx = g->pull == TUG_PULL_PERFECT ? -12 : -8;
    } else if (lost_pull && g->pull == TUG_PULL_SLIP) {
        doe = &tug_doe_slip;
        uint32_t slide = g->result == TUG_RESULT_LOSE ? TUG_RESOLVE_MS : LV_MIN(g->phase_ms * 2, TUG_RESOLVE_MS);
        dx = (int32_t)(SLIP_SLIDE * slide / TUG_RESOLVE_MS);
        mouth_dy = TUG_DOE_SLIP_DROP * DOE_SCALE;
        hand_dx = 12;
        hand_dy = -4;
    } else if (lost_pull) {
        dx = 8;
        hand_dx = 12;
        hand_dy = -4;
    } else if (tense) {
        int32_t drag;
        hand_dx = owner_pull(g, t, &drag);
        tension = LV_MAX(hand_dx, 0) / 2;
        // 中、重力度时 Doe 压低重心顶住；摆动时力量越大身体越往后仰
        if (g->force != TUG_FORCE_LIGHT && hand_dx > 0) doe = &tug_doe_guard;
        dx = drag;
        if (g->phase == TUG_PHASE_SWING) dx -= (int32_t)tug_meter(g) * 4 / TUG_POWER_MAX;
    } else {
        dy = (t / 400) % 2 ? -2 : 0;
    }
    if (doe != s_doe_sprite) {
        sprite_draw(s_doe, doe, DOE_SCALE);
        s_doe_sprite = doe;
    }
    lv_obj_set_pos(s_doe, DOE_X + dx, DOE_TOP + dy);
    int32_t mouth_x = MOUTH_X + dx;
    int32_t mouth_y = ROPE_Y + dy + mouth_dy;

    int32_t hand_x = HAND_X + hand_dx;
    int32_t hand_y = ROPE_Y + hand_dy;
    lv_obj_set_pos(s_hand, hand_x, hand_y - HAND_H / 2);
    int32_t sleeve_x = hand_x + HAND_W - 4;
    lv_obj_set_pos(s_sleeve, sleeve_x, hand_y - SLEEVE_H / 2);
    lv_obj_set_width(s_sleeve, 240 + 4 - sleeve_x);

    int32_t target = toy_target_x(g->cell);
    if (s_toy_x < target) s_toy_x = LV_MIN(s_toy_x + TOY_SPEED, target);
    if (s_toy_x > target) s_toy_x = LV_MAX(s_toy_x - TOY_SPEED, target);
    int32_t toy_x = s_toy_x + tension;
    int32_t toy_y = mouth_y + (hand_y - mouth_y) * (toy_x - mouth_x) / LV_MAX(hand_x - mouth_x, 1);
    if (g->result == TUG_RESULT_WIN && s_toy_x == target) {
        toy_x = mouth_x;
        toy_y = mouth_y;
    }
    lv_obj_set_pos(s_toy, toy_x - TUG_TOY_SIZE * TOY_SCALE / 2, toy_y - TUG_TOY_SIZE * TOY_SCALE / 2);

    s_rope_points[0] = (lv_point_precise_t){mouth_x, mouth_y};
    s_rope_points[1] = (lv_point_precise_t){toy_x, toy_y};
    s_rope_points[2] = (lv_point_precise_t){hand_x + 4, hand_y};
    lv_line_set_points(s_rope, s_rope_points, 3);
}

static void show_result(void) {
    const TugGame* g = &s_game;
    TugReward reward = tug_reward(g->result, g->misses);
    AppCmd cmd = {
        .type = APP_CMD_GAME_RESULT,
        .result = {.game = GAME_TUG, .stars = reward.stars, .shells = reward.shells, .spirit = reward.spirit},
    };
    app_cmd_send(&cmd);

    bool won = g->result == TUG_RESULT_WIN;
    lv_label_set_text(s_result_title, won ? "胜利" : "失败");
    lv_obj_set_style_text_color(s_result_title, lv_color_hex(won ? UI_COLOR_GLOW : UI_COLOR_ALERT), 0);
    lv_label_set_text_fmt(s_result_desc, "%s  失误 %u 次", won ? "抢到了玩具" : "玩具被抢走了",
                          (unsigned)g->misses);
    lv_label_set_text_fmt(s_result_stars, "%s%s%s", reward.stars >= 1 ? "★" : "☆",
                          reward.stars >= 2 ? "★" : "☆", reward.stars >= 3 ? "★" : "☆");
    lv_label_set_text_fmt(s_result_shells, "贝壳 +%u", (unsigned)reward.shells);
    lv_obj_set_hidden(s_result, false);
    lv_obj_set_hidden(s_feedback, true);
    lv_label_set_text(s_hint_label, "确定:再来  返回:菜单");

    s_result_shown = true;
    s_result_at = lv_tick_get();
    printf("[Tug] %s，%u 轮，失误 %u 次，完美 %u 次，%u 星，用时 %.1f 秒\n", won ? "胜利" : "失败",
           (unsigned)g->round, (unsigned)g->misses, (unsigned)g->perfects, (unsigned)reward.stars,
           g->elapsed_ms / 1000.0);
}

static void advance(void) {
    uint32_t now = lv_tick_get();
    uint32_t dt = now - s_last_tick;
    s_last_tick = now;
    tug_step(&s_game, dt > MAX_STEP_MS ? MAX_STEP_MS : dt);
}

static void on_frame(lv_timer_t* timer) {
    (void)timer;
    advance();
    render();
    if (s_game.result != TUG_RESULT_NONE && !s_result_shown) show_result();
}

static void start(void) {
    tug_init(&s_game, (uint32_t)hal_clock_us());
    s_last_tick = lv_tick_get();
    s_result_shown = false;
    s_shown_round = 0;
    s_shown_phase = TUG_PHASE_OVER;
    s_shown_count = 0xFF;
    s_toy_x = toy_target_x(s_game.cell);
    s_track_toy_x = track_x(s_game.cell);
    lv_obj_set_x(s_track_toy, s_track_toy_x - TUG_TOY_SIZE * TRACK_TOY_SCALE / 2);
    lv_obj_set_hidden(s_result, true);
    lv_label_set_text(s_hint_label, "确定:用力  返回:退出");
    render();
}

static void create_track(lv_obj_t* parent) {
    int32_t x_end = track_x(TUG_CELLS - 1);
    lv_obj_t* rope = rect(parent, COLOR_ROPE);
    lv_obj_set_pos(rope, TRACK_X0, TRACK_Y - 1);
    lv_obj_set_size(rope, x_end - TRACK_X0, 3);

    // 中间的标记是短竖线，起点稍长；两端是各自颜色的终点桩
    for (int i = 1; i < TUG_CELLS - 1; i++) {
        int32_t h = i == TUG_CELL_START ? 10 : 6;
        lv_obj_t* tick = rect(parent, i == TUG_CELL_START ? UI_COLOR_TEXT : UI_COLOR_TEXT_DIM);
        lv_obj_set_pos(tick, track_x(i) - 1, TRACK_Y - h / 2);
        lv_obj_set_size(tick, 2, h);
    }
    lv_obj_t* doe_end = rect(parent, UI_COLOR_GLOW);
    lv_obj_set_pos(doe_end, TRACK_X0 - 2, TRACK_Y - 8);
    lv_obj_set_size(doe_end, 4, 16);
    lv_obj_t* owner_end = rect(parent, UI_COLOR_ALERT);
    lv_obj_set_pos(owner_end, x_end - 2, TRACK_Y - 8);
    lv_obj_set_size(owner_end, 4, 16);

    lv_obj_t* doe = label(parent, &font_ui_12, UI_COLOR_GLOW);
    lv_label_set_text(doe, "DOE");
    lv_obj_align(doe, LV_ALIGN_TOP_RIGHT, -(240 - TRACK_X0 + 10), TRACK_Y - 7);
    lv_obj_t* owner = label(parent, &font_ui_12, UI_COLOR_ALERT);
    lv_label_set_text(owner, "主人");
    lv_obj_set_pos(owner, x_end + 10, TRACK_Y - 7);

    int32_t toy = TUG_TOY_SIZE * TRACK_TOY_SCALE;
    s_track_toy = sprite_canvas_create(parent, &s_track_toy_buf, toy, toy);
    sprite_draw(s_track_toy, &tug_toy, TRACK_TOY_SCALE);
    lv_obj_set_y(s_track_toy, TRACK_Y - toy / 2);
}

static void create_hud(lv_obj_t* parent) {
    s_round = label(parent, &font_ui_24, UI_COLOR_TEXT);
    lv_obj_align(s_round, LV_ALIGN_TOP_MID, 0, 14);
    create_track(parent);

    s_feedback = label(parent, &font_ui_12, UI_COLOR_TEXT);
    lv_obj_align(s_feedback, LV_ALIGN_TOP_MID, 0, 72);

    lv_obj_t* hint = ui_hint_bar_create(parent, "");
    lv_obj_set_size(hint, UI_CONTENT_W, 24);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -UI_SAFE_EDGE);
    s_hint_label = lv_obj_get_child(hint, 0);
}

static void create_meter(lv_obj_t* parent) {
    lv_obj_t* title = label(parent, &font_ui_12, UI_COLOR_TEXT_DIM);
    lv_label_set_text(title, "力量");
    lv_obj_set_pos(title, METER_X, METER_Y - 18);

    lv_obj_t* track = rect(parent, UI_COLOR_PANEL);
    lv_obj_set_style_border_width(track, 2, 0);
    lv_obj_set_style_border_color(track, lv_color_hex(UI_COLOR_LINE), 0);
    lv_obj_set_pos(track, METER_X, METER_Y);
    lv_obj_set_size(track, METER_W, METER_H);

    // 绿色是反拉成功的范围，中间亮的一段是完美，跟着主人拉力每轮移动、一轮比一轮窄
    s_match_band = rect(parent, UI_COLOR_LINE_DIM);
    s_perfect_band = rect(parent, UI_COLOR_GLOW);

    s_owner_pointer = outlined(parent, UI_COLOR_ALERT);
    lv_obj_set_size(s_owner_pointer, POINTER_W, POINTER_H);
    lv_obj_set_y(s_owner_pointer, METER_Y + (METER_H - POINTER_H) / 2);
    s_pointer = outlined(parent, UI_COLOR_FOCUS);
    lv_obj_set_size(s_pointer, POINTER_W, POINTER_H);
    lv_obj_set_y(s_pointer, METER_Y + (METER_H - POINTER_H) / 2);
}

static void create_countdown(lv_obj_t* scr) {
    s_digit = sprite_canvas_create(scr, &s_digit_buf, GAME_DIGIT_W * DIGIT_SCALE, GAME_DIGIT_H * DIGIT_SCALE);
    lv_obj_align(s_digit, LV_ALIGN_TOP_MID, 0, 50);
    s_go = label(scr, &font_ui_24, UI_COLOR_GLOW);
    lv_label_set_text(s_go, "开始!");
    lv_obj_align(s_go, LV_ALIGN_TOP_MID, 0, 62);
}

static void create_field(lv_obj_t* scr) {
    lv_obj_t* ground = rect(scr, UI_COLOR_LINE_DIM);
    lv_obj_set_pos(ground, UI_SAFE_EDGE, GROUND_Y);
    lv_obj_set_size(ground, UI_CONTENT_W, 2);

    s_rope = lv_line_create(scr);
    lv_obj_set_style_line_width(s_rope, 3, 0);
    lv_obj_set_style_line_color(s_rope, lv_color_hex(COLOR_ROPE), 0);

    s_doe = sprite_canvas_create(scr, &s_doe_buf, TUG_DOE_W * DOE_SCALE, TUG_DOE_H * DOE_SCALE);
    s_doe_sprite = NULL;

    s_sleeve = outlined(scr, COLOR_SLEEVE);
    lv_obj_set_style_radius(s_sleeve, 0, 0);
    lv_obj_set_height(s_sleeve, SLEEVE_H);
    s_hand = outlined(scr, COLOR_SKIN);
    lv_obj_set_size(s_hand, HAND_W, HAND_H);

    s_toy = sprite_canvas_create(scr, &s_toy_buf, TUG_TOY_SIZE * TOY_SCALE, TUG_TOY_SIZE * TOY_SCALE);
    sprite_draw(s_toy, &tug_toy, TOY_SCALE);
}

static void create_result(lv_obj_t* scr) {
    s_result = ui_panel_create(scr);
    lv_obj_set_size(s_result, 184, 124);
    lv_obj_align(s_result, LV_ALIGN_CENTER, 0, 4);
    lv_obj_set_flex_flow(s_result, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_result, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_result, 4, 0);
    s_result_title = label(s_result, &font_ui_24, UI_COLOR_GLOW);
    s_result_desc = label(s_result, &font_ui_12, UI_COLOR_TEXT_DIM);
    s_result_stars = label(s_result, &font_ui_24, UI_COLOR_FOCUS);
    s_result_shells = label(s_result, &font_ui_12, UI_COLOR_TEXT);
}

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    ui_screen_dark(scr);
    s_hud = layer(scr);
    create_hud(s_hud);
    s_meter = layer(scr);
    create_meter(s_meter);
    create_countdown(scr);
    create_field(scr);
    create_result(scr);

    start();
    s_timer = lv_timer_create(on_frame, FRAME_MS, NULL);
}

static void destroy(void) {
    if (s_game.result == TUG_RESULT_NONE) printf("[Tug] 中途退出，没有奖励\n");
    if (s_timer) lv_timer_delete(s_timer);
    s_timer = NULL;
}

static bool on_input(const InputEvent* e) {
    if (e->key == INPUT_KEY_BACK) return false;
    if (s_game.result != TUG_RESULT_NONE) {
        if (e->type == INPUT_EVT_SHORT_PRESS && e->key == INPUT_KEY_OK &&
            lv_tick_elaps(s_result_at) >= RESULT_LOCK_MS) {
            start();
        }
        return true;
    }
    if (e->type == INPUT_EVT_PRESS && e->key == INPUT_KEY_OK) {
        // 按键在两帧之间到达，先把规则推进到现在再锁定，读数才准
        advance();
        tug_lock(&s_game);
        render();
    }
    return true;
}

const Page page_tug = {
    .name = "tug",
    .create = create,
    .destroy = destroy,
    .on_input = on_input,
    .idle_timeout_ms = 0,
};
