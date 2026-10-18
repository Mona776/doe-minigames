/**
 * 按键读取 - Mac 模拟器：键盘 A=选择 S=确定 D=返回 空格=触摸
 * 键盘状态由 LVGL 的 SDL 驱动在 lv_timer_handler() 里刷新
 * 设置环境变量 SIM_NO_KEYBOARD 时不读键盘，只认终端注入的按键（脚本截图时窗口抢到焦点也不会被误按打乱）
 */

#include <SDL2/SDL.h>
#include <stdlib.h>
#include "hal/hal_input.h"
#include "input/input.h"

static bool s_keyboard = true;

void hal_input_init(void) {
    s_keyboard = getenv("SIM_NO_KEYBOARD") == NULL;
}

uint32_t hal_input_read(void) {
    if (!s_keyboard) return 0;
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    uint32_t mask = 0;
    if (keys[SDL_SCANCODE_A])     mask |= INPUT_BIT(INPUT_KEY_SELECT);
    if (keys[SDL_SCANCODE_S])     mask |= INPUT_BIT(INPUT_KEY_OK);
    if (keys[SDL_SCANCODE_D])     mask |= INPUT_BIT(INPUT_KEY_BACK);
    if (keys[SDL_SCANCODE_SPACE]) mask |= INPUT_BIT(INPUT_KEY_TOUCH);
    return mask;
}
