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

#define DOE_Y        99
#define BEST_Y       130
#define OPTION_W    138
#define OPTION_H    26
#define OPTION_Y0   144
#define OPTION_STEP 32

enum { OPTION_START, OPTION_HELP, OPTION_COUNT };

static const char* const OPTION_TEXT[OPTION_COUNT] = {"开始游戏", "玩法说明"};
static const uint8_t FOCUS_HELP = OPTION_HELP;  // 从说明页回来时传给入口页，选中玩法说明

static lv_draw_buf_t* s_title_bone_buf;

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

static void update_focus(void) {
    for (uint8_t i = 0; i < OPTION_COUNT; i++) sausage_ui_focus(s_options[i], i == s_focus);
}

// arg 为空时选中开始游戏，否则指向要选中的选项
static void entry_create(lv_obj_t* scr, void* arg) {
    sausage_ui_screen(scr, true);
    lv_obj_t* title = label(scr, &font_sausage_24, SAUSAGE_UI_GOLD, "SAUSAGE");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);
    title = label(scr, &font_sausage_24, SAUSAGE_UI_TEXT, "SNAKE");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);
    title = label(scr, &font_sausage_12, SAUSAGE_UI_DIM, "ENDLESS");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 84);
    lv_obj_t* doe = sausage_ui_doe(scr, 2);
    lv_obj_align(doe, LV_ALIGN_TOP_MID, 0, DOE_Y);
    lv_obj_t* bone = snake_icon_create(scr, &s_title_bone_buf, &snake_icon_bone, 1);
    lv_obj_align(bone, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_t* best = label(scr, &font_sausage_12, SAUSAGE_UI_DIM, "");
    lv_label_set_text_fmt(best, "最高纪录 %u", (unsigned)ui_records_best(GAME_SAUSAGE));
    lv_obj_align(best, LV_ALIGN_TOP_MID, 0, BEST_Y);
    for (uint8_t i = 0; i < OPTION_COUNT; i++) {
        s_options[i] = sausage_ui_option(scr, OPTION_TEXT[i], &font_sausage_12, OPTION_W, OPTION_H);
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

static void help_create(lv_obj_t* scr, void* arg) {
    (void)arg;
    sausage_ui_screen(scr, true);
    lv_obj_t* title = label(scr, &font_sausage_24, SAUSAGE_UI_TEXT, "玩法说明");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_t* key = sausage_ui_keycap(scr, 'B');
    lv_obj_set_pos(key, 48, 57);
    lv_obj_t* doe = sausage_ui_doe(scr, 2);
    lv_obj_set_pos(doe, 87, 53);
    lv_obj_t* turn = label(scr, &font_sausage_12, SAUSAGE_UI_GOLD, "按确认键，下一步右转");
    lv_obj_align(turn, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_t* line = ui_box_create(scr);
    lv_obj_set_size(line, 192, 1);
    lv_obj_set_pos(line, 24, 116);
    lv_obj_set_style_bg_color(line, lv_color_hex(SAUSAGE_UI_EDGE), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    static const char* const rules[] = {
        "每2根骨头，身体长1格",
        "撞墙、障碍或自己就结束",
        "越吃越快，障碍逐步增加",
        "5 / 10 / 15 根点亮星星",
    };
    for (uint8_t i = 0; i < 4; i++) {
        uint32_t color = i == 3 ? SAUSAGE_UI_GOLD : i == 2 ? SAUSAGE_UI_DIM : SAUSAGE_UI_TEXT;
        lv_obj_t* l = label(scr, &font_sausage_12, color, rules[i]);
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 127 + i * 21);
    }
    hint_bar(scr, "B 开始  C 返回");
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
