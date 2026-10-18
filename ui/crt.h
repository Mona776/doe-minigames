/**
 * CRT 显示效果 - 像素发给屏幕前统一加工：四周机壳（圆角屏幕开口）、开口内侧的绿色发光线、扫描线
 * 所有页面都带，页面布局要离屏幕边缘至少 UI_SAFE_EDGE（ui_theme.h）
 * 只在 LVGL 所在的线程/任务中使用
 */

#ifndef CRT_H
#define CRT_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

// 挂到显示器的 LV_EVENT_FLUSH_START，之后每次刷屏自动加工
void crt_init(lv_display_t* disp);

// 扫描线开关（机壳一直在）
void crt_set_scanlines(bool on);
bool crt_scanlines_enabled(void);

// 加工一块 RGB565 像素；px 指向 area 左上角，stride_px 是一行的像素数，area 用屏幕坐标
void crt_apply(uint16_t* px, int32_t stride_px, const lv_area_t* area);

#endif
