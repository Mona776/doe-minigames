/**
 * 界面侧记住的各小游戏最高分 - 由逻辑服务的 APP_EVT_BEST_SCORE 事件更新，页面随时可读
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef UI_RECORDS_H
#define UI_RECORDS_H

#include <stdint.h>
#include "core/app_msg.h"

void ui_records_on_event(const AppEvent* e);

uint16_t ui_records_best(GameId game);

#endif
