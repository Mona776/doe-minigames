/**
 * Mac 模拟器的时钟与系统抽象实现（单线程，临界区为空操作）
 */

#include <time.h>
#include "hal/hal_clock.h"
#include "hal/hal_sys.h"

uint64_t hal_clock_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

uint64_t hal_clock_ms(void) {
    return hal_clock_us() / 1000u;
}

bool hal_clock_wall_time(int64_t* epoch_local) {
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    *epoch_local = (int64_t)now + local.tm_gmtoff;
    return true;
}

void hal_critical_enter(void) {}
void hal_critical_exit(void) {}
