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
    , grams_error_treshold(NAN)
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

extern "C" bool loadcell_get_state() {
    return loadcell.GetState();
}

bool Loadcell::GetState() const {
    return endstop;
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

void Loadcell::SetHighPrecisionEnabled(bool enable) {
    this->highPrecision = enable;
}

bool Loadcell::IsHighPrecisionEnabled() const {
    return highPrecision;
}

void Loadcell::SetGramsErrorTreshold(float grams_error_treshold) {
    this->grams_error_treshold = grams_error_treshold;
}

float Loadcell::GetGramsErrorTreshold() const {
    return grams_error_treshold;
}

void Loadcell::ProcessSample(int32_t loadcellRaw) {
    if (!isSignalEventConfigured) {
        return;
    }

    this->loadcellRaw = loadcellRaw;
    float load = scale * (loadcellRaw - offset); //scale to gram

    metric_record_integer(&metric_loadcell_raw, loadcellRaw);
    metric_record_float(&metric_loadcell, load);

    if ((!std::isnan(grams_error_treshold)) && (load > grams_error_treshold))
        general_error("loadcell", "grams error treshold reached");

    if (endstop) {
        if (load >= (threshold + hysteresis)) {
            endstop = false;
        }
    } else {
        if (load <= threshold) {
            endstop = true;
            endstops_poll();
        }
    }
    osSignalSet(threadId, signal);
}

//creates object enforcing error when pozitive load value is too big
Loadcell::TempErrEnforcer Loadcell::CreateErrEnforcer(float grams) {
    return Loadcell::TempErrEnforcer(*this, grams);
}

/*****************************************************************************/
//TempErrEnforcer
Loadcell::TempErrEnforcer::TempErrEnforcer(Loadcell &lcell, float grams_error_treshold)
    : lcell(lcell)
    , old_grams_error_treshold(lcell.GetGramsErrorTreshold()) {
    lcell.SetGramsErrorTreshold(grams_error_treshold);
}

Loadcell::TempErrEnforcer::~TempErrEnforcer() {
    lcell.SetGramsErrorTreshold(old_grams_error_treshold);
}
