#pragma once

#ifdef __cplusplus
    #include <inttypes.h>
    #include "cmsis_os.h"

class Loadcell {
public:
    Loadcell();

    void Tare(int tareCount = 4);

    void SetScale(float scale);

    float GetScale() const;

    void SetThreshold(float threshold);

    float GetThreshold() const;

    void SetHysteresis(float hysteresis);

    float GetHysteresis() const;

    void ProcessSample(int32_t loadcellRaw);

    bool GetState() const;

    void ConfigureSignalEvent(osThreadId threadId, int32_t signal);

    // return loadcell load in grams
    float GetLoad() const;

    int32_t GetRawValue() const;

    bool IsSignalConfigured() const;

    void SetHighPrecisionEnabled(bool enable);

    bool IsHighPrecisionEnabled() const;

private:
    float scale;
    float threshold;
    float hysteresis;
    uint8_t state;
    int32_t offset;
    osThreadId threadId;
    int32_t signal;
    int32_t loadcellRaw;
    bool isSignalEventConfigured;
    bool highPrecision;
};
    #define EXTERN_C extern "C"
extern Loadcell loadcell;

#else
    #define EXTERN_C
#endif

#include <stdbool.h>

EXTERN_C bool loadcell_get_state();
