/**
 * Pico 的时钟与系统抽象实现
 */

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hal/hal_clock.h"
#include "hal/hal_sys.h"

uint64_t hal_clock_us(void) {
    return time_us_64();
}

uint64_t hal_clock_ms(void) {
    return time_us_64() / 1000u;
}

// 还没接 PCF8563 实时时钟，没有可信的墙上时间
bool hal_clock_wall_time(int64_t* epoch_local) {
    (void)epoch_local;
    return false;
}

void hal_critical_enter(void) {
    taskENTER_CRITICAL();
}

void hal_critical_exit(void) {
    taskEXIT_CRITICAL();
}
