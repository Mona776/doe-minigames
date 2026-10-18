/**
 * 首页 - 选择或确定打开菜单
 */

#include "pages.h"
#include "ui/ui_theme.h"

static void create(lv_obj_t* scr, void* arg) {
    (void)arg;
    ui_screen_dark(scr);
    lv_obj_t* title = ui_title_create(scr, "小游戏");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_SAFE_EDGE);

    lv_obj_t* hint = ui_hint_bar_create(scr, "按键:菜单");
    lv_obj_set_size(hint, UI_CONTENT_W, 24);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -UI_SAFE_EDGE);
}

static bool on_input(const InputEvent* e) {
    if (e->type == INPUT_EVT_SHORT_PRESS &&
        (e->key == INPUT_KEY_SELECT || e->key == INPUT_KEY_OK)) {
        page_push(&page_menu, NULL);
        return true;
    }
    return false;
}

const Page page_home = {
    .name = "home",
    .create = create,
    .on_input = on_input,
};
