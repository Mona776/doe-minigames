/**
 * 按键读取 - FreeRTOS 模拟器
 * 主线程读键盘，写进 sim_host。界面任务从这里取，不直接碰 SDL。
 * SIM_NO_KEYBOARD 时不读键盘，只认终端注入的按键。
 */

#include "hal/hal_input.h"
#include "sim_host.h"
#include <stdlib.h>

static bool s_keyboard = true;

void hal_input_init(void) {
    s_keyboard = getenv("SIM_NO_KEYBOARD") == NULL;
}

uint32_t hal_input_read(void) {
    if (!s_keyboard) return 0;
    return sim_keys_read();
}
