#pragma once

#include <inttypes.h>

class HX711 {

public:
    enum Channel {
        CHANNEL_A_GAIN_128 = 1,
        CHANNEL_A_GAIN_64 = 3,
        CHANNEL_B_GAIN_32 = 2
    };
    HX711();
    int32_t ReadValue(Channel configureChannel);
    bool IsValueReady();
    void Configure(int dataPin, int clockPin);

private:
    int dataPin;
    int clockPin;
    bool isConfigured;
};

extern HX711 hx711;
