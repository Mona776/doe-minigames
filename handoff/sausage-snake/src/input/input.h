/**
 * 输入层 - 把原始按键电平翻译成按键事件（纯 C，不依赖平台）
 *
 * 物理键：选择、确定、返回
 *   按下：消抖后立即触发一次，按住不重复；要求手感的小游戏用它，菜单等仍用短按
 *   短按：松开时触发（按住不足长按阈值）
 *   长按：按住达到阈值时触发一次，之后松开不再产生短按
 *   组合键：一个键按住时按下另一个键，立即触发组合键，这两个键在松开前不再产生单键事件
 * 触摸键：按下、松开（带持续时长），按住达到阈值时额外触发一次长按
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INPUT_KEY_SELECT = 0,
    INPUT_KEY_OK,
    INPUT_KEY_BACK,
    INPUT_KEY_TOUCH,
    INPUT_KEY_COUNT
} InputKey;

#define INPUT_BIT(key) (1u << (key))

typedef enum {
    INPUT_EVT_SHORT_PRESS,
    INPUT_EVT_LONG_PRESS,
    INPUT_EVT_COMBO,
    INPUT_EVT_TOUCH_DOWN,
    INPUT_EVT_TOUCH_UP,
    INPUT_EVT_PRESS,
} InputEventType;

typedef enum {
    INPUT_COMBO_SELECT_OK,    // 死亡后重新领养
    INPUT_COMBO_OK_BACK,      // 静音切换
    INPUT_COMBO_SELECT_BACK,  // 时间设置里数值减一
} InputCombo;

typedef struct {
    InputEventType type;
    InputKey key;          // 组合键事件中无意义
    InputCombo combo;      // 仅组合键事件有效
    uint32_t duration_ms;  // 短按、长按、触摸松开时为按住时长
} InputEvent;

void input_init(void);

// 周期调用（建议 5ms），raw_mask 为当前原始电平（INPUT_BIT 组合，1 表示按下）
void input_feed(uint32_t raw_mask, uint32_t now_ms);

bool input_poll(InputEvent* out);

// 模拟按住一组键 hold_ms 毫秒（调试命令和测试脚本使用），与真实按键叠加
void input_inject(uint32_t mask, uint32_t hold_ms);

const char* input_key_name(InputKey key);
const char* input_combo_name(InputCombo combo);

#endif
