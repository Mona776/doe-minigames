/**
 * 输入层单元测试（在 Mac 上运行：./run_tests.sh）
 */

#include <stdbool.h>
#include <stdio.h>
#include "input/input.h"
#include "input/input_config.h"

static int s_failures;
static uint32_t s_now;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("  ✗ %s（第 %d 行）\n", msg, __LINE__);            \
            s_failures++;                                             \
        }                                                             \
    } while (0)

static const uint32_t SEL = INPUT_BIT(INPUT_KEY_SELECT);
static const uint32_t OK = INPUT_BIT(INPUT_KEY_OK);
static const uint32_t BACK = INPUT_BIT(INPUT_KEY_BACK);
static const uint32_t TOUCH = INPUT_BIT(INPUT_KEY_TOUCH);

static void reset(void) {
    input_init();
    s_now = 1000;
    input_feed(0, s_now);
}

// 以 5ms 步长保持 mask 一段时间
static void hold(uint32_t mask, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 5) {
        s_now += 5;
        input_feed(mask, s_now);
    }
}

// 按下事件单独在 test_press 里检查，其余用例只看短按、长按、组合键、触摸
static int drain_all(InputEvent* events, int max, bool with_press) {
    int n = 0;
    InputEvent e;
    while (input_poll(&e)) {
        if (!with_press && e.type == INPUT_EVT_PRESS) continue;
        if (n < max) events[n] = e;
        n++;
    }
    return n;
}

static int drain(InputEvent* events, int max) {
    return drain_all(events, max, false);
}

static void test_short_press(void) {
    printf("短按\n");
    reset();
    hold(SEL, 200);
    hold(0, 50);
    InputEvent ev[8];
    int n = drain(ev, 8);
    CHECK(n == 1, "应只产生 1 个事件");
    CHECK(ev[0].type == INPUT_EVT_SHORT_PRESS && ev[0].key == INPUT_KEY_SELECT, "应为选择键短按");
    CHECK(ev[0].duration_ms >= 190 && ev[0].duration_ms <= 210, "时长约 200ms");
}

static void test_bounce_ignored(void) {
    printf("抖动过滤\n");
    reset();
    for (int i = 0; i < 3; i++) {
        hold(OK, 5);
        hold(0, 5);
    }
    hold(0, 50);
    InputEvent ev[8];
    CHECK(drain(ev, 8) == 0, "短于消抖时间的抖动不应产生事件");
}

static void test_long_press(void) {
    printf("长按\n");
    reset();
    hold(BACK, INPUT_LONG_PRESS_MS - 100);
    InputEvent ev[8];
    CHECK(drain(ev, 8) == 0, "未到阈值不应触发");
    hold(BACK, 200);
    int n = drain(ev, 8);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_LONG_PRESS && ev[0].key == INPUT_KEY_BACK, "到阈值触发一次长按");
    hold(BACK, 3000);
    CHECK(drain(ev, 8) == 0, "继续按住不再重复触发");
    hold(0, 50);
    CHECK(drain(ev, 8) == 0, "长按后松开不产生短按");
}

static void test_combo(void) {
    printf("组合键\n");
    reset();
    hold(SEL, 100);
    hold(SEL | OK, 100);
    InputEvent ev[8];
    int n = drain(ev, 8);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_COMBO && ev[0].combo == INPUT_COMBO_SELECT_OK, "选择+确定");
    hold(SEL | OK, INPUT_LONG_PRESS_MS + 100);
    hold(OK, 100);
    hold(0, 50);
    CHECK(drain(ev, 8) == 0, "组合键后的长按和松开都不产生单键事件");

    reset();
    hold(OK | BACK, 100);
    n = drain(ev, 8);
    CHECK(n == 1 && ev[0].combo == INPUT_COMBO_OK_BACK, "同时按下确定+返回");

    reset();
    hold(BACK, 50);
    hold(BACK | SEL, 50);
    n = drain(ev, 8);
    CHECK(n == 1 && ev[0].combo == INPUT_COMBO_SELECT_BACK, "选择+返回（顺序无关）");
}

static void test_no_combo_after_long(void) {
    printf("长按后再按另一键\n");
    reset();
    hold(SEL, INPUT_LONG_PRESS_MS + 50);
    hold(SEL | OK, 100);
    hold(SEL, 50);
    hold(0, 50);
    InputEvent ev[8];
    int n = drain(ev, 8);
    CHECK(n == 2, "应为长按 + 确定短按");
    CHECK(ev[0].type == INPUT_EVT_LONG_PRESS && ev[0].key == INPUT_KEY_SELECT, "先长按选择");
    CHECK(ev[1].type == INPUT_EVT_SHORT_PRESS && ev[1].key == INPUT_KEY_OK, "再短按确定");
}

static void test_touch(void) {
    printf("触摸\n");
    reset();
    hold(TOUCH, 500);
    hold(0, 50);
    InputEvent ev[8];
    int n = drain(ev, 8);
    CHECK(n == 2 && ev[0].type == INPUT_EVT_TOUCH_DOWN && ev[1].type == INPUT_EVT_TOUCH_UP, "按下+松开");
    CHECK(ev[1].duration_ms >= 490 && ev[1].duration_ms <= 510, "松开带时长");

    reset();
    hold(TOUCH, INPUT_TOUCH_HOLD_MS + 50);
    hold(0, 50);
    n = drain(ev, 8);
    CHECK(n == 3 && ev[1].type == INPUT_EVT_LONG_PRESS && ev[1].key == INPUT_KEY_TOUCH, "持续触摸 3 秒触发长按");

    reset();
    hold(TOUCH | SEL, 100);
    hold(0, 50);
    n = drain(ev, 8);
    CHECK(n == 3, "触摸不参与组合键：按下、选择短按、松开");
}

static void test_press(void) {
    printf("按下即触发\n");
    reset();
    hold(OK, INPUT_DEBOUNCE_MS + 10);
    InputEvent ev[8];
    int n = drain_all(ev, 8, true);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_PRESS && ev[0].key == INPUT_KEY_OK, "消抖后不等松开就触发按下");
    hold(OK, INPUT_LONG_PRESS_MS + 1000);
    n = drain_all(ev, 8, true);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_LONG_PRESS, "按住不重复触发按下，只有一次长按");

    reset();
    for (int i = 0; i < 5; i++) {
        hold(SEL, 40);
        hold(0, 40);
    }
    InputEvent taps[16];
    n = drain_all(taps, 16, true);
    int presses = 0;
    for (int i = 0; i < n && i < 16; i++) presses += taps[i].type == INPUT_EVT_PRESS;
    CHECK(presses == 5, "连按 5 次产生 5 次按下");

    reset();
    hold(TOUCH, 100);
    n = drain_all(ev, 8, true);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_TOUCH_DOWN, "触摸键不产生按下事件");
}

static void test_inject(void) {
    printf("模拟按键注入\n");
    reset();
    input_inject(OK, 150);
    hold(0, 300);
    InputEvent ev[8];
    int n = drain(ev, 8);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_SHORT_PRESS && ev[0].key == INPUT_KEY_OK, "注入短按");

    reset();
    input_inject(SEL, INPUT_LONG_PRESS_MS + 100);
    hold(0, INPUT_LONG_PRESS_MS + 300);
    n = drain(ev, 8);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_LONG_PRESS, "注入长按");

    reset();
    input_inject(SEL | OK, 100);
    hold(0, 200);
    n = drain(ev, 8);
    CHECK(n == 1 && ev[0].type == INPUT_EVT_COMBO, "注入组合键");
}

int main(void) {
    test_short_press();
    test_bounce_ignored();
    test_long_press();
    test_combo();
    test_no_combo_after_long();
    test_touch();
    test_press();
    test_inject();

    if (s_failures == 0) {
        printf("✅ 输入层测试全部通过\n");
        return 0;
    }
    printf("❌ %d 项失败\n", s_failures);
    return 1;
}
