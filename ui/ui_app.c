/**
 * 界面总入口
 */

#include "ui_app.h"
#include <stdio.h>
#include "hal/hal_input.h"
#include "input/input.h"
#include "ui/crt.h"
#include "ui/page_mgr.h"
#include "ui/pages/pages.h"
#include "ui/ui_events.h"
#include "ui/ui_records.h"

void ui_app_init(void) {
    crt_init(lv_display_get_default());
    page_mgr_init(&page_home);
    ui_events_subscribe(ui_records_on_event);
    ui_events_subscribe(page_mgr_on_event);
}

static void log_input(const InputEvent* e) {
    switch (e->type) {
        case INPUT_EVT_SHORT_PRESS:
            printf("[Key] %s 短按 %ums\n", input_key_name(e->key), (unsigned)e->duration_ms);
            break;
        case INPUT_EVT_LONG_PRESS:
            printf("[Key] %s 长按\n", input_key_name(e->key));
            break;
        case INPUT_EVT_COMBO:
            printf("[Key] 组合键 %s\n", input_combo_name(e->combo));
            break;
        case INPUT_EVT_TOUCH_DOWN:
            printf("[Key] 触摸按下\n");
            break;
        case INPUT_EVT_TOUCH_UP:
            printf("[Key] 触摸松开 %ums\n", (unsigned)e->duration_ms);
            break;
        case INPUT_EVT_PRESS:
            break;
    }
}

void ui_app_tick(uint32_t now_ms) {
    input_feed(hal_input_read(), now_ms);
    InputEvent e;
    while (input_poll(&e)) {
        log_input(&e);
        page_mgr_handle_input(&e, now_ms);
    }
    ui_events_dispatch();
    page_mgr_tick(now_ms);
}
