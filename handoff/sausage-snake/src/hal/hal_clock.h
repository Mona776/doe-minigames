/**
 * 时钟硬件抽象 - 各平台在 platform/<平台>/ 下实现
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

// 单调递增的开机时间
uint64_t hal_clock_ms(void);
uint64_t hal_clock_us(void);

// 当前本地墙上时间（从 1970-01-01 00:00:00 起的秒数，不含时区换算）
// 没有可信时间来源（例如 Pico 还没接实时时钟）时返回 false
bool hal_clock_wall_time(int64_t* epoch_local);

#endif
