#include "loadcell.h"
#include "loadcell_endstops.h"
#include "gpio.h"
#include "metric.h"

Loadcell loadcell;
static metric_t metric_loadcell_raw = METRIC("loadcell_raw", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);
static metric_t metric_loadcell = METRIC("loadcell", METRIC_VALUE_FLOAT, 0, METRIC_HANDLER_DISABLE_ALL);

Loadcell::Loadcell() {
}

void Loadcell::ConfigureSignalEvent(osThreadId threadId, int32_t signal) {
    this->threadId = threadId;
    this->signal = signal;
    isSignalEventConfigured = 1;
}

void Loadcell::Tare(int tareCount) {
    if (!isSignalEventConfigured) {
        return;
    }

    osEvent evt;
    int32_t sum = 0;
    int tareSamples = tareCount;

    while (tareSamples--) {
        evt = osSignalWait(signal, 100);
        if (evt.status == osEventSignal) {
            sum += loadcellRaw;
        }
    }
    offset = sum / tareCount;
}

extern "C" bool loadcell_get_state() {
    return loadcell.GetState();
}

bool Loadcell::GetState() const {
    return state;
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

void Loadcell::ProcessSample(int32_t loadcellRaw) {
    if (!isSignalEventConfigured) {
        return;
    }

    this->loadcellRaw = loadcellRaw;
    float load = scale * (loadcellRaw - offset); //scale to gram

    metric_record_integer(&metric_loadcell_raw, loadcellRaw);
    metric_record_float(&metric_loadcell, load);

    if (state) {
        if (load >= (threshold + hysteresis)) {
            state = false;
        }
    } else {
        if (load <= threshold) {
            state = true;
            endstops_poll();
        }
    }
    osSignalSet(threadId, signal);
}
