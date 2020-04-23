#pragma once

#ifdef __cplusplus
    #include <inttypes.h>
    #include "cmsis_os.h"
    #include <array>

class Loadcell {
public:
    enum class TareMode {
        Continuous,
        Static,
    };

    Loadcell();

    void Tare(TareMode mode = TareMode::Static);

    void SetScale(float scale);
    float GetScale() const;

    void SetThreshold(float threshold);
    float GetThreshold() const;

    void SetHysteresis(float hysteresis);
    float GetHysteresis() const;

    void ProcessSample(int32_t loadcellRaw);

    bool GetMinZEndstop() const;
    bool GetMaxZEndstop() const;

    inline void EnableZMaxEndstopOnInactiveZMin() { triggerZmaxOnInactiveZmin = true; }
    inline void DisableZMaxEndstopOnInactiveZMin() { triggerZmaxOnInactiveZmin = false; }

    void ConfigureSignalEvent(osThreadId threadId, int32_t signal);

    // return loadcell load in grams
    float GetLoad() const;

    int32_t GetRawValue() const;

    bool IsSignalConfigured() const;

    inline void EnableHighPrecision(bool wait = true) {
        highPrecision = true;
        if (wait)
            WaitForNextSample();
    }
    inline void DisableHighPrecision() { highPrecision = false; }
    inline bool IsHighPrecisionEnabled() const { return highPrecision; }

    void SetFailsOnLoadAbove(float failsOnLoadAbove);
    float GetFailsOnLoadAbove() const;

    void SetFailsOnLoadBelow(float failsOnLoadBelow);
    float GetFailsOnLoadBelow() const;

    class IFailureEnforcer {
    protected:
        Loadcell &lcell;
        float oldErrThreshold;
        IFailureEnforcer(Loadcell &lcell, float oldErrThreshold);
        IFailureEnforcer(const IFailureEnforcer &) = delete;
        IFailureEnforcer(IFailureEnforcer &&) = default;
    };

    class FailureOnLoadAboveEnforcer : public IFailureEnforcer {
    public:
        FailureOnLoadAboveEnforcer(Loadcell &lcell, float grams);
        FailureOnLoadAboveEnforcer(FailureOnLoadAboveEnforcer &&) = default;
        ~FailureOnLoadAboveEnforcer();
    };

    class FailureOnLoadBelowEnforcer : public IFailureEnforcer {
    public:
        FailureOnLoadBelowEnforcer(Loadcell &lcell, float grams);
        FailureOnLoadBelowEnforcer(FailureOnLoadBelowEnforcer &&) = default;
        ~FailureOnLoadBelowEnforcer();
    };

    FailureOnLoadAboveEnforcer CreateLoadAboveErrEnforcer(float grams = 500);

    FailureOnLoadBelowEnforcer CreateLoadBelowErrEnforcer(float grams = -3000);

private:
    bool triggerZmaxOnInactiveZmin;
    float scale;
    float threshold;
    float hysteresis;
    float failsOnLoadAbove;
    float failsOnLoadBelow;
    osThreadId threadId;
    int32_t signal;
    int32_t loadcellRaw;
    bool endstop;
    bool isSignalEventConfigured;
    bool highPrecision;

    TareMode tareMode;
    // used when tareMode == Static
    int32_t offset;
    // used when tareMode == Continuous
    std::array<int32_t, 32> window;

    int32_t WaitForNextSample();
};

    #define EXTERN_C extern "C"
extern Loadcell loadcell;

#else
    #define EXTERN_C
#endif

#include <stdbool.h>

EXTERN_C bool loadcell_get_min_z_endstop();
EXTERN_C bool loadcell_get_max_z_endstop();
