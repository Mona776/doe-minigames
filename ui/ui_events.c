/**
 * 界面侧的事件分发
 */

#include "ui_events.h"

#define MAX_SUBSCRIBERS 8

static UiEventHandler s_handlers[MAX_SUBSCRIBERS];
static int s_handler_count;

void ui_events_subscribe(UiEventHandler handler) {
    if (s_handler_count < MAX_SUBSCRIBERS) s_handlers[s_handler_count++] = handler;
}

void ui_events_dispatch(void) {
    AppEvent evt;
    while (app_event_receive(&evt)) {
        for (int i = 0; i < s_handler_count; i++) s_handlers[i](&evt);
    }
}
