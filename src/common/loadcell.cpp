#include "loadcell.h"
#include "loadcell_endstops.h"
#include "gpio.h"
#include "metric.h"
#include "bsod.h"
#include <cmath> //isnan

Loadcell loadcell;
static metric_t metric_loadcell_raw = METRIC("loadcell_raw", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);
static metric_t metric_loadcell = METRIC("loadcell", METRIC_VALUE_FLOAT, 0, METRIC_HANDLER_DISABLE_ALL);
static metric_t metric_tare_err = METRIC("tare_err", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_ENABLE_ALL);

Loadcell::Loadcell()
    : scale(1)
    , threshold(NAN)
    , hysteresis(0)
    , failsOnLoadAbove(INFINITY)
    , failsOnLoadBelow(-INFINITY)
    , offset(0)
    , loadcellRaw(0)
    , endstop(false)
    , isSignalEventConfigured(false)
    , highPrecision(false) {
}

void Loadcell::ConfigureSignalEvent(osThreadId threadId, int32_t signal) {
    this->threadId = threadId;
    this->signal = signal;
    isSignalEventConfigured = 1;
}

void Loadcell::Tare(int tareCount) {
    if (!isSignalEventConfigured) {
        general_error("loadcell", "uncomplete configuration");
        return;
    }

    int32_t sum = 0;
    int measuredCount = 0;
    int errors = 0;

    while (errors < 3 && measuredCount < tareCount) {
        osEvent evt = osSignalWait(signal, 100);
        if (evt.status == osEventSignal) {
            sum += loadcellRaw;
            measuredCount += 1;
        } else {
            metric_record_integer(&metric_tare_err, (int)evt.status);
            errors += 1;
        }
    }

    if (measuredCount == tareCount) {
        offset = sum / measuredCount;
    } else {
        general_error("loadcell", "tare failed");
    }
}

bool Loadcell::GetMinZEndstop() const {
    return endstop;
}

bool Loadcell::GetMaxZEndstop() const {
    return triggerZmaxOnInactiveZmin ? !endstop : false;
}

void Loadcell::SetTriggerZMaxOnInactiveZMin(bool enabled) {
    triggerZmaxOnInactiveZmin = enabled;
}

void Loadcell::SetScale(float scale) {
    this->scale = scale;
}

void Loadcell::SetThreshold(float threshold) {
    this->threshold = threshold;
}

void Loadcell::SetHysteresis(float hysteresis) {
    this->hysteresis = hysteresis;
}

float Loadcell::GetScale() const {
    return scale;
}

float Loadcell::GetThreshold() const {
    return threshold;
}

float Loadcell::GetHysteresis() const {
    return hysteresis;
}

float Loadcell::GetLoad() const {
    return (scale * (loadcellRaw - offset));
}

int32_t Loadcell::GetRawValue() const {
    return loadcellRaw;
}

bool Loadcell::IsSignalConfigured() const {
    return isSignalEventConfigured;
}

void Loadcell::SetHighPrecisionEnabled(bool enable, bool wait) {
    this->highPrecision = enable;
    if (wait && isSignalEventConfigured) {
        auto result = osSignalWait(signal, 600);
        if (result.status != osEventSignal) {
            general_error("loadcell", "timeout when waiting for high precision mode");
        }
    }
}

bool Loadcell::IsHighPrecisionEnabled() const {
    return highPrecision;
}

void Loadcell::SetFailsOnLoadAbove(float failsOnLoadAbove) {
    this->failsOnLoadAbove = failsOnLoadAbove;
}

float Loadcell::GetFailsOnLoadAbove() const {
    return failsOnLoadAbove;
}

void Loadcell::SetFailsOnLoadBelow(float failsOnLoadBelow) {
    this->failsOnLoadBelow = failsOnLoadBelow;
}

float Loadcell::GetFailsOnLoadBelow() const {
    return failsOnLoadBelow;
}

void Loadcell::ProcessSample(int32_t loadcellRaw) {
    if (!isSignalEventConfigured) {
        return;
    }

    this->loadcellRaw = loadcellRaw;
    float load = scale * (loadcellRaw - offset); //scale to gram

    metric_record_integer(&metric_loadcell_raw, loadcellRaw);
    metric_record_float(&metric_loadcell, load);

    if (!std::isfinite(load))
        general_error("loadcell", "grams error, not finite");
    if (load > failsOnLoadAbove)
        general_error("loadcell", "grams error threshold reached");
    if (load < failsOnLoadBelow)
        general_error("loadcell", "grams error threshold reached");

    if (endstop) {
        if (load >= (threshold + hysteresis)) {
            endstop = false;
            endstops_poll();
        }
    } else {
        if (load <= threshold) {
            endstop = true;
            endstops_poll();
        }
    }
    osSignalSet(threadId, signal);
}

//creates object enforcing error when positive load value is too big
Loadcell::FailureOnLoadAboveEnforcer Loadcell::CreateLoadAboveErrEnforcer(float grams) {
    return Loadcell::FailureOnLoadAboveEnforcer(*this, grams);
}

Loadcell::FailureOnLoadBelowEnforcer Loadcell::CreateLoadBelowErrEnforcer(float grams) {
    return Loadcell::FailureOnLoadBelowEnforcer(*this, grams);
}

extern "C" bool loadcell_get_max_z_endstop() {
    return loadcell.GetMaxZEndstop();
}

extern "C" bool loadcell_get_min_z_endstop() {
    return loadcell.GetMinZEndstop();
}

/*****************************************************************************/
//IFailureEnforcer
Loadcell::IFailureEnforcer::IFailureEnforcer(Loadcell &lcell, float oldErrThreshold)
    : lcell(lcell)
    , oldErrThreshold(oldErrThreshold) {
}

/*****************************************************************************/
//FailureOnLoadAboveEnforcer
Loadcell::FailureOnLoadAboveEnforcer::FailureOnLoadAboveEnforcer(Loadcell &lcell, float grams)
    : IFailureEnforcer(lcell, lcell.GetFailsOnLoadAbove()) {
    lcell.SetFailsOnLoadAbove(grams);
}
Loadcell::FailureOnLoadAboveEnforcer::~FailureOnLoadAboveEnforcer() {
    lcell.SetFailsOnLoadAbove(oldErrThreshold);
}

/*****************************************************************************/
//FailureOnLoadBelowEnforcer
Loadcell::FailureOnLoadBelowEnforcer::FailureOnLoadBelowEnforcer(Loadcell &lcell, float grams)
    : IFailureEnforcer(lcell, lcell.GetFailsOnLoadBelow()) {
    lcell.SetFailsOnLoadBelow(grams);
}
Loadcell::FailureOnLoadBelowEnforcer::~FailureOnLoadBelowEnforcer() {
    lcell.SetFailsOnLoadBelow(oldErrThreshold);
}
