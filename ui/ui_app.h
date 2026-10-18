/**
 * 界面总入口 - 串起页面、输入和事件分发
 * 需在 lv_init、显示器创建、input_init、hal_input_init 之后调用
 */

#ifndef UI_APP_H
#define UI_APP_H

#include <stdint.h>

void ui_app_init(void);

// 在 LVGL 线程中周期调用（建议 5~10ms 一次）
void ui_app_tick(uint32_t now_ms);

#endif
