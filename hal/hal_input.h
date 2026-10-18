/**
 * 按键原始电平读取 - 各平台实现，结果交给 input_feed() 判定
 */

#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include <stdint.h>

void hal_input_init(void);

// 返回 INPUT_BIT(INPUT_KEY_xxx) 组合，1 表示当前按下
uint32_t hal_input_read(void);

#endif
