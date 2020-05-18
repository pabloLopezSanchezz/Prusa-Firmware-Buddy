#include "ext_mux.h"
#include "gpio.h"

static int _pin_select_a;
static int _pin_select_b;
static int _pin_channel_a;
static int _pin_channel_b;

void ext_mux_configure(int pin_select_a, int pin_select_b, int pin_channel_a, int pin_channel_b) {
    _pin_select_a = pin_select_a;
    _pin_select_b = pin_select_b;
    _pin_channel_a = pin_channel_a;
    _pin_channel_b = pin_channel_b;

    gpio_init(_pin_select_a, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(_pin_select_b, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_set(_pin_select_a, 0);
    gpio_set(_pin_select_b, 0);
}

void ext_mux_select(int logic_input) {
    switch (logic_input) {
    case 0:
        gpio_set(_pin_select_a, 0);
        gpio_set(_pin_select_b, 0);
        break;
    case 1:
        gpio_set(_pin_select_a, 1);
        gpio_set(_pin_select_b, 0);
        break;
    case 2:
        gpio_set(_pin_select_a, 0);
        gpio_set(_pin_select_b, 1);
        break;
    case 3:
        gpio_set(_pin_select_a, 1);
        gpio_set(_pin_select_b, 1);
        break;
    }
}
