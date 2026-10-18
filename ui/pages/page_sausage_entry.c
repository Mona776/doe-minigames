/**
 * Sausage Snake 入口页和玩法说明页
 * 入口：游戏名、Doe、历史最高、「开始游戏」「玩法说明」两个选项（默认开始游戏）；A 切换，B 确定，C 回小游戏菜单
 * 说明：转向示意加几句规则；B 开始游戏，C 回入口（选中玩法说明）
 * 入口、说明、游戏三页互相替换（page_replace），栈里只有 小游戏菜单 > 当前页，同一时间只占一页的内存
 */

#include "pages.h"
#include "ui/ui_records.h"
#include "ui/ui_theme.h"
#include "snake_art.h"
#include "snake_ui.h"
#include "sausage_ui.h"

#define DOE_SCALE    3
#define DOE_BONE_GAP 2
#define DOE_W        (SNAKE_LEGS - SNAKE_TILE + 5 * SNAKE_TILE + DOE_BONE_GAP)  // 四节身体、一根骨头，两边各留脚伸出的 1 像素
#define DOE_Y        94
#define BEST_Y       132
#define OPTION_W    138
#define OPTION_H    26
#define OPTION_Y0   150
#define OPTION_STEP 33
#define ICON_SCALE  2
#define TURN_PX     (SNAKE_TURN_ICON * ICON_SCALE)
#define HELP_X      24
#define HELP_TEXT_X (HELP_X + TURN_PX + 8)

enum { OPTION_START, OPTION_HELP, OPTION_COUNT };

static const char* const OPTION_TEXT[OPTION_COUNT] = {"开始游戏", "玩法说明"};
static const uint8_t FOCUS_HELP = OPTION_HELP;  // 从说明页回来时传给入口页，选中玩法说明

static lv_draw_buf_t* s_doe_buf;
static lv_draw_buf_t* s_turn_buf;

static lv_obj_t* s_options[OPTION_COUNT];
static uint8_t s_focus;

static lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t color, const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, text);
    return l;
}

static void hint_bar(lv_obj_t* scr, const char* text) {
    sausage_ui_hint(scr, text);
}

static void place(lv_obj_t* obj, int32_t x, int32_t y) {
    lv_obj_set_pos(obj, x, y);
}

// 侧面的 Doe：尾巴、两节身体、头，前后脚挂在头和尾巴下面，头前面一根骨头。
// 先按原尺寸拼成一张小图再整体放大，分开放大的几张图接缝处会露出细缝
static void create_doe(lv_obj_t* scr) {
    int32_t inset = (SNAKE_LEGS - SNAKE_TILE) / 2;
    lv_draw_buf_t* buf = sprite_buf_begin(&s_doe_buf, DOE_W, SNAKE_LEGS);
    sprite_buf_paint_at(buf, &snake_tail, inset, inset);
    sprite_buf_paint_at(buf, &snake_body, inset + SNAKE_TILE, inset);
    sprite_buf_paint_at(buf, &snake_body, inset + 2 * SNAKE_TILE, inset);
    sprite_buf_paint_at(buf, &snake_head, inset + 3 * SNAKE_TILE, inset);
    sprite_buf_paint_at(buf, &snake_bone, inset + 4 * SNAKE_TILE + DOE_BONE_GAP, inset);
    sprite_buf_paint_at(buf, &snake_back_legs[0], 0, 0);
    sprite_buf_paint_at(buf, &snake_front_legs[0], 3 * SNAKE_TILE, 0);

    lv_obj_t* doe = lv_image_create(scr);
    lv_image_set_src(doe, buf);
    lv_image_set_antialias(doe, false);
    lv_image_set_inner_align(doe, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_set_size(doe, DOE_W * DOE_SCALE, SNAKE_LEGS * DOE_SCALE);
    lv_obj_align(doe, LV_ALIGN_TOP_MID, 0, DOE_Y);
}

static void update_focus(void) {
    for (uint8_t i = 0; i < OPTION_COUNT; i++) sausage_ui_focus(s_options[i], i == s_focus);
}

// arg 为空时选中开始游戏，否则指向要选中的选项
static void entry_create(lv_obj_t* scr, void* arg) {
    sausage_ui_screen(scr, true);
    lv_obj_t* title = label(scr, &font_ui_24, SAUSAGE_UI_GOLD, "SAUSAGE");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);
    title = label(scr, &font_ui_24, SAUSAGE_UI_TEXT, "SNAKE");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 52);
    title = label(scr, &font_ui_12, SAUSAGE_UI_DIM, "ENDLESS");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 78);
    create_doe(scr);
    lv_obj_t* best = label(scr, &font_ui_12, SAUSAGE_UI_DIM, "");
    lv_label_set_text_fmt(best, "最高纪录 %u", (unsigned)ui_records_best(GAME_SAUSAGE));
    lv_obj_align(best, LV_ALIGN_TOP_MID, 0, BEST_Y);
    for (uint8_t i = 0; i < OPTION_COUNT; i++) {
        s_options[i] = sausage_ui_option(scr, OPTION_TEXT[i], &font_ui_12, OPTION_W, OPTION_H);
        lv_obj_align(s_options[i], LV_ALIGN_TOP_MID, 0, OPTION_Y0 + i * OPTION_STEP);
    }
    hint_bar(scr, "A 切换  B 确定  C 返回");
    s_focus = arg ? *(const uint8_t*)arg : OPTION_START;
    update_focus();
}

static bool entry_on_input(const InputEvent* e) {
    if (e->type != INPUT_EVT_SHORT_PRESS) return false;
    if (e->key == INPUT_KEY_SELECT) {
        s_focus = (uint8_t)((s_focus + 1) % OPTION_COUNT);
        update_focus();
        return true;
    }
    if (e->key == INPUT_KEY_OK) {
        page_replace(s_focus == OPTION_START ? &page_sausage : &page_sausage_help, NULL);
        return true;
    }
    return false;
}

const Page page_sausage_entry = {
    .name = "sausage_entry",
    .create = entry_create,
    .on_input = entry_on_input,
    .idle_timeout_ms = PAGE_IDLE_TIMEOUT_DEFAULT,
};

static const char* const RULE_TIPS[] = {
    "每吃两根骨头，身体长一节",
    "撞墙、石头或自己就结束",
    "越吃越快，石头越来越多",
    "5 / 10 / 15 骨头点亮星星",
    "满三星后继续玩，挑战最高分",
};

#define RULE_N (sizeof(RULE_TIPS) / sizeof(RULE_TIPS[0]))

static void help_create(lv_obj_t* scr, void* arg) {
    (void)arg;
    sausage_ui_screen(scr, true);
    lv_obj_t* title = label(scr, &font_ui_24, SAUSAGE_UI_TEXT, "玩法说明");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_SAFE_EDGE);

    place(snake_icon_create(scr, &s_turn_buf, &snake_turn_right, ICON_SCALE), HELP_X, 68);
    lv_obj_t* turn = label(scr, &font_ui_12, SAUSAGE_UI_TEXT, "每按一次 B，向右转90°");
    lv_obj_set_pos(turn, HELP_TEXT_X, 71);
    lv_obj_t* line = ui_box_create(scr);
    lv_obj_set_size(line, UI_CONTENT_W - 2 * (HELP_X - UI_SAFE_EDGE), 2);
    lv_obj_set_pos(line, HELP_X, 114);
    lv_obj_set_style_bg_color(line, lv_color_hex(SAUSAGE_UI_EDGE), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    for (uint8_t i = 0; i < RULE_N; i++) {
        lv_obj_t* l = label(scr, &font_ui_12, SAUSAGE_UI_DIM, RULE_TIPS[i]);
        lv_obj_set_pos(l, HELP_X, 120 + i * 16);
    }
    hint_bar(scr, "B 开始游戏  C 返回");
}

static bool help_on_input(const InputEvent* e) {
    if (e->type != INPUT_EVT_SHORT_PRESS) return false;
    if (e->key == INPUT_KEY_OK) {
        page_replace(&page_sausage, NULL);
        return true;
    }
    if (e->key == INPUT_KEY_BACK) {
        page_replace(&page_sausage_entry, (void*)&FOCUS_HELP);
        return true;
    }
    return false;
}

const Page page_sausage_help = {
    .name = "sausage_help",
    .create = help_create,
    .on_input = help_on_input,
    .idle_timeout_ms = PAGE_IDLE_TIMEOUT_DEFAULT,
};
