/**
 * 界面侧的事件分发：取出逻辑层发来的事件，按订阅顺序交给各订阅者
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include "core/app_msg.h"

typedef void (*UiEventHandler)(const AppEvent* evt);

void ui_events_subscribe(UiEventHandler handler);
void ui_events_dispatch(void);

#endif
