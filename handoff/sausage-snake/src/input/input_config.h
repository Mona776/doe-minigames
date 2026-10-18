/**
 * 输入判定阈值（对应 PRD 数值80 等）
 */

#ifndef INPUT_CONFIG_H
#define INPUT_CONFIG_H

#define INPUT_DEBOUNCE_MS        20    // 原始电平稳定这么久才认
#define INPUT_LONG_PRESS_MS      2000  // 物理键长按（PRD 数值80）
#define INPUT_TOUCH_HOLD_MS      3000  // 触摸持续按住（孵化"捂蛋"等）
#define INPUT_EVENT_QUEUE_LEN    16

#endif
