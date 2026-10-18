/**
 * 自定义字体声明 - 由 lv_conf.h 通过 LV_FONT_CUSTOM_INCLUDE 引入
 * 缝合像素字体的 1 位色字库，只用 12 的整数倍字号
 * font_ui_12 是正文，font_ui_24 是标题和菜单项
 */

#ifndef PET_FONTS_H
#define PET_FONTS_H

#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(font_ui_12) LV_FONT_DECLARE(font_ui_24)
#define LV_FONT_DEFAULT        &font_ui_12

#endif
