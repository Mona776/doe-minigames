/**
 * 游戏菜单 - 选择键移动焦点，确定键打开游戏页面，返回键回到首页
 * 新游戏在 s_items 哨兵之前加一行即可
 */

#include "pages.h"
#include "ui/ui_theme.h"

#define ITEM_Y0   48
#define ITEM_STEP 39
#define ITEM_W    188
#define ITEM_H    36
#define VISIBLE   4
#define BAR_X     (120 + ITEM_W / 2 + 5)
#define BAR_W     3
#define BAR_H     (VISIBLE * ITEM_STEP - (ITEM_STEP - ITEM_H))

typedef struct {
    const char* label;
    const char* icon;
    const Page* page;
} MenuItem;

static const MenuItem s_items[] = {
    {"拔河", NULL, &page_tug},
    {"贪吃腊肠", NULL, &page_snake_entry},
    {"接网球", NULL, &page_tennis},
    {"Sausage Snake", NULL, &page_sausage_entry},
    {NULL, NULL, NULL},
};

static uint32_t item_count(void) {
    uint32_t n = 0;
    while (s_items[n].label) n++;
    return n;
}

static lv_obj_t* s_rows[8];
static lv_obj_t* s_thumb;
static uint32_t s_focus;
static uint32_t s_top;

static void update_focus(uint32_t count) {
    if (s_focus < s_top) s_top = s_focus;
    if (s_focus >= s_top + VISIBLE) s_top = s_focus - (VISIBLE - 1);
    for (uint32_t i = 0; i < count; i++) {
        bool shown = i >= s_top && i < s_top + VISIBLE;
        lv_obj_set_hidden(s_rows[i], !shown);
        if (shown) lv_obj_set_y(s_rows[i], ITEM_Y0 + (int32_t)(i - s_top) * ITEM_STEP);
        ui_button_set_focused(s_rows[i], i == s_focus);
    }
    if (s_thumb) lv_obj_set_y(s_thumb, ITEM_Y0 + BAR_H * (int32_t)s_top / (int32_t)count);
}

static lv_obj_t* create_bar_part(lv_obj_t* scr, int32_t h, uint32_t color) {
    lv_obj_t* part = lv_obj_create(scr);
    lv_obj_remove_style_all(part);
    lv_obj_set_size(part, BAR_W, h);
    lv_obj_set_style_bg_color(part, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(part, LV_OPA_COVER, 0);
    lv_obj_set_pos(part, BAR_X, ITEM_Y0);
    return part;
}

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    ui_screen_dark(scr);
    lv_obj_t* title = ui_title_create(scr, "小游戏");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_SAFE_EDGE);

    uint32_t count = item_count();
    if (count > sizeof(s_rows) / sizeof(s_rows[0])) count = sizeof(s_rows) / sizeof(s_rows[0]);

    if (count == 0) {
        lv_obj_t* empty = lv_label_create(scr);
        lv_label_set_text(empty, "还没游戏");
        lv_obj_set_style_text_color(empty, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
        lv_obj_center(empty);
    }

    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t* row = ui_button_create(scr, s_items[i].icon, s_items[i].label);
        lv_obj_set_size(row, ITEM_W, ITEM_H);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, ITEM_Y0 + (int32_t)i * ITEM_STEP);
        s_rows[i] = row;
    }
    s_thumb = NULL;
    if (count > VISIBLE) {
        create_bar_part(scr, BAR_H, UI_COLOR_LINE_DIM);
        s_thumb = create_bar_part(scr, BAR_H * VISIBLE / (int32_t)count, UI_COLOR_LINE);
    }

    lv_obj_t* hint = ui_hint_bar_create(scr, count ? "选择:下一项  确定:执行" : "返回:首页");
    lv_obj_set_size(hint, UI_CONTENT_W, 24);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -UI_SAFE_EDGE);

    s_focus = 0;
    s_top = 0;
    if (count) update_focus(count);
}

static bool on_input(const InputEvent* e) {
    uint32_t count = item_count();
    if (count > sizeof(s_rows) / sizeof(s_rows[0])) count = sizeof(s_rows) / sizeof(s_rows[0]);
    if (count == 0 || e->type != INPUT_EVT_SHORT_PRESS) return false;
    if (e->key == INPUT_KEY_SELECT) {
        s_focus = (s_focus + 1) % count;
        update_focus(count);
        return true;
    }
    if (e->key == INPUT_KEY_OK) {
        const Page* page = s_items[s_focus].page;
        if (page) page_push(page, NULL);
        return true;
    }
    return false;
}

const Page page_menu = {
    .name = "menu",
    .create = create,
    .on_input = on_input,
    .idle_timeout_ms = PAGE_IDLE_TIMEOUT_DEFAULT,
};
