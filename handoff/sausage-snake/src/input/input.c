/**
 * 输入层 - 消抖、短按/长按/组合键/触摸判定
 */

#include "input.h"
#include <string.h>
#include "input_config.h"

#define PHYSICAL_MASK (INPUT_BIT(INPUT_KEY_SELECT) | INPUT_BIT(INPUT_KEY_OK) | INPUT_BIT(INPUT_KEY_BACK))

typedef struct {
    uint32_t down_since;
    bool long_fired;
    bool suppressed;  // 参与了组合键，松开前不再产生单键事件
} KeyState;

static struct {
    uint32_t stable_mask;
    uint32_t candidate_mask;
    uint32_t candidate_since;
    uint32_t now;
    KeyState keys[INPUT_KEY_COUNT];

    uint32_t inject_until[INPUT_KEY_COUNT];
    uint32_t inject_mask;

    InputEvent queue[INPUT_EVENT_QUEUE_LEN];
    uint32_t head, tail;
} s;

void input_init(void) {
    memset(&s, 0, sizeof(s));
}

static void push(InputEventType type, InputKey key, InputCombo combo, uint32_t duration) {
    if (s.head - s.tail >= INPUT_EVENT_QUEUE_LEN) return;
    InputEvent* e = &s.queue[s.head++ % INPUT_EVENT_QUEUE_LEN];
    e->type = type;
    e->key = key;
    e->combo = combo;
    e->duration_ms = duration;
}

static bool combo_of(InputKey a, InputKey b, InputCombo* out) {
    uint32_t pair = INPUT_BIT(a) | INPUT_BIT(b);
    if (pair == (INPUT_BIT(INPUT_KEY_SELECT) | INPUT_BIT(INPUT_KEY_OK)))   { *out = INPUT_COMBO_SELECT_OK;   return true; }
    if (pair == (INPUT_BIT(INPUT_KEY_OK) | INPUT_BIT(INPUT_KEY_BACK)))     { *out = INPUT_COMBO_OK_BACK;     return true; }
    if (pair == (INPUT_BIT(INPUT_KEY_SELECT) | INPUT_BIT(INPUT_KEY_BACK))) { *out = INPUT_COMBO_SELECT_BACK; return true; }
    return false;
}

static void on_key_down(InputKey key, uint32_t down_mask) {
    KeyState* k = &s.keys[key];
    k->down_since = s.now;
    k->long_fired = false;
    k->suppressed = false;

    if (key == INPUT_KEY_TOUCH) {
        push(INPUT_EVT_TOUCH_DOWN, key, 0, 0);
        return;
    }
    push(INPUT_EVT_PRESS, key, 0, 0);

    for (int other = 0; other < INPUT_KEY_COUNT; other++) {
        if (other == (int)key || !(down_mask & INPUT_BIT(other)) || !(PHYSICAL_MASK & INPUT_BIT(other))) continue;
        KeyState* o = &s.keys[other];
        InputCombo combo;
        if (!o->suppressed && !o->long_fired && combo_of((InputKey)other, key, &combo)) {
            o->suppressed = true;
            k->suppressed = true;
            push(INPUT_EVT_COMBO, key, combo, 0);
            return;
        }
    }
}

static void on_key_up(InputKey key) {
    KeyState* k = &s.keys[key];
    uint32_t duration = s.now - k->down_since;
    if (key == INPUT_KEY_TOUCH) {
        push(INPUT_EVT_TOUCH_UP, key, 0, duration);
    } else if (!k->suppressed && !k->long_fired) {
        push(INPUT_EVT_SHORT_PRESS, key, 0, duration);
    }
}

void input_feed(uint32_t raw_mask, uint32_t now_ms) {
    s.now = now_ms;

    for (int key = 0; key < INPUT_KEY_COUNT; key++) {
        if ((s.inject_mask & INPUT_BIT(key)) && (int32_t)(now_ms - s.inject_until[key]) >= 0) {
            s.inject_mask &= ~INPUT_BIT(key);
        }
    }
    uint32_t mask = (raw_mask | s.inject_mask) & ((1u << INPUT_KEY_COUNT) - 1);

    if (mask != s.candidate_mask) {
        s.candidate_mask = mask;
        s.candidate_since = now_ms;
    }

    if (s.candidate_mask != s.stable_mask && now_ms - s.candidate_since >= INPUT_DEBOUNCE_MS) {
        uint32_t pressed = s.candidate_mask & ~s.stable_mask;
        uint32_t released = s.stable_mask & ~s.candidate_mask;
        uint32_t down_mask = s.stable_mask & ~released;

        for (int key = 0; key < INPUT_KEY_COUNT; key++) {
            if (released & INPUT_BIT(key)) on_key_up((InputKey)key);
        }
        for (int key = 0; key < INPUT_KEY_COUNT; key++) {
            if (pressed & INPUT_BIT(key)) {
                down_mask |= INPUT_BIT(key);
                on_key_down((InputKey)key, down_mask);
            }
        }
        s.stable_mask = s.candidate_mask;
    }

    for (int key = 0; key < INPUT_KEY_COUNT; key++) {
        KeyState* k = &s.keys[key];
        if (!(s.stable_mask & INPUT_BIT(key)) || k->suppressed || k->long_fired) continue;
        uint32_t threshold = key == INPUT_KEY_TOUCH ? INPUT_TOUCH_HOLD_MS : INPUT_LONG_PRESS_MS;
        uint32_t held = now_ms - k->down_since;
        if (held >= threshold) {
            k->long_fired = true;
            push(INPUT_EVT_LONG_PRESS, (InputKey)key, 0, held);
        }
    }
}

bool input_poll(InputEvent* out) {
    if (s.head == s.tail) return false;
    *out = s.queue[s.tail++ % INPUT_EVENT_QUEUE_LEN];
    return true;
}

void input_inject(uint32_t mask, uint32_t hold_ms) {
    for (int key = 0; key < INPUT_KEY_COUNT; key++) {
        if (mask & INPUT_BIT(key)) s.inject_until[key] = s.now + hold_ms;
    }
    s.inject_mask |= mask & ((1u << INPUT_KEY_COUNT) - 1);
}

const char* input_key_name(InputKey key) {
    switch (key) {
        case INPUT_KEY_SELECT: return "select";
        case INPUT_KEY_OK:     return "ok";
        case INPUT_KEY_BACK:   return "back";
        case INPUT_KEY_TOUCH:  return "touch";
        default:               return "?";
    }
}

const char* input_combo_name(InputCombo combo) {
    switch (combo) {
        case INPUT_COMBO_SELECT_OK:   return "select+ok";
        case INPUT_COMBO_OK_BACK:     return "ok+back";
        case INPUT_COMBO_SELECT_BACK: return "select+back";
        default:                      return "?";
    }
}
