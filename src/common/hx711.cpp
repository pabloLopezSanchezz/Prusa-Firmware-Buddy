#include "hx711.h"
#include "gpio.h"
#include "config_a3ides2209_02.h"
#include "cmsis_os.h"

HX711 hx711;

HX711::HX711() {
}

void HX711::Configure(int dataPin, int clockPin) {
    this->dataPin = dataPin;
    this->clockPin = clockPin;
    gpio_init(dataPin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
    gpio_init(clockPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_set(clockPin, 0);
    isConfigured = true;
}

int32_t HX711::ReadValue(Channel nextChannel) {
    int index;
    int32_t result = 0;
    for (index = 0; index < 24; index++) {
        gpio_set(clockPin, 1);
        gpio_set(clockPin, 1);
        result = (result << 1) | gpio_get(dataPin);
        gpio_set(clockPin, 0);
    }

    for (int index = 0; index < nextChannel; index++) {
        gpio_set(clockPin, 1);
        gpio_set(clockPin, 1);
        gpio_set(clockPin, 0);
    }
    return (result >= 0x800000) ? (result | 0xFF000000) : result;
}

bool HX711::IsValueReady() {
    if (!isConfigured) {
        return 0;
    }
    return gpio_get(dataPin) ? 0 : 1;
}
