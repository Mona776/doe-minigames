/**
 * 按键读取 - Pico GPIO
 */

#include "pico/stdlib.h"
#include "board_pins.h"
#include "hal/hal_input.h"
#include "input/input.h"

static void init_key(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

void hal_input_init(void) {
    init_key(PIN_KEY_SELECT);
    init_key(PIN_KEY_OK);
    init_key(PIN_KEY_BACK);

    gpio_init(PIN_TOUCH);
    gpio_set_dir(PIN_TOUCH, GPIO_IN);
    gpio_pull_down(PIN_TOUCH);
}

uint32_t hal_input_read(void) {
    uint32_t mask = 0;
    if (!gpio_get(PIN_KEY_SELECT)) mask |= INPUT_BIT(INPUT_KEY_SELECT);
    if (!gpio_get(PIN_KEY_OK))     mask |= INPUT_BIT(INPUT_KEY_OK);
    if (!gpio_get(PIN_KEY_BACK))   mask |= INPUT_BIT(INPUT_KEY_BACK);
    if (gpio_get(PIN_TOUCH))       mask |= INPUT_BIT(INPUT_KEY_TOUCH);
    return mask;
}
