#include "advanced_power.h"
#include "hwio.h"
#include "metric.h"
#include "adc.h"
#include "bsod.h"

AdvancedPower advancedpower;

AdvancedPower::AdvancedPower()
    : rawBedVoltage(0)
    , rawHeaterVoltage(0)
    , rawHeaterCurrent(0)
    , rawInputCurrent(0)
    , heaterOvercurentFaultDetected(false)
    , overcurrentFaultDetected(false) {
}

bool AdvancedPower::HeaterOvercurentFaultDetected() const {
    return heaterOvercurentFaultDetected;
}

bool AdvancedPower::OvercurrentFaultDetected() const {
    return overcurrentFaultDetected;
}

void AdvancedPower::ResetOvercurrentFault() {
    adc_reset_fault_signal();
}

void AdvancedPower::Update() {
    rawBedVoltage = adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_A, EXT_mux_select_t::BED_VOLTAGE);
    rawHeaterCurrent = adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_B, EXT_mux_select_t::HEATER_CURRENT);
    rawHeaterVoltage = adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_A, EXT_mux_select_t::HEATER_VOLTAGE);
    rawInputCurrent = adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_B, EXT_mux_select_t::INPUT_CURRENT);

    if (adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_B, EXT_mux_select_t::HEATER_FAULT) > 900) {
        heaterOvercurentFaultDetected = true;
        general_error("overcurrent", "Overcurrent detected on nozzle");
    } else {
        heaterOvercurentFaultDetected = false;
    }

    if (adc_ext_mux_get_raw(EXT_mux_channel_t::MUX_CHANNEL_B, EXT_mux_select_t::CURRENT_FAULT) > 900) {
        overcurrentFaultDetected = true;
        general_error("overcurrent", "Overcurrent detected on input");
    } else {
        overcurrentFaultDetected = false;
    }
}
