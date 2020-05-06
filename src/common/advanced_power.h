#pragma once
#include "ext_mux.h"

class AdvancedPower {
public:
    AdvancedPower();

    inline float GetBedVoltage() const {
        float R1 = 10.0 * 1000.0;
        float R2 = 1.0 * 1000.0;
        return RawValueToVoltage(rawBedVoltage) / (R2 / (R1 + R2));
    }

    inline int GetBedVoltageRaw() const {
        return rawBedVoltage;
    }

    inline float GetHeaterVoltage() const {
        float R1 = 10.0 * 1000.0;
        float R2 = 1.0 * 1000.0;
        return RawValueToVoltage(rawHeaterVoltage) / (R2 / (R1 + R2));
    }

    inline int GetHeaterVoltageRaw() const {
        return rawHeaterVoltage;
    }

    inline float GetHeaterCurrent() const {
        float mvToAmper = 0.022;
        return (RawValueToVoltage(rawHeaterCurrent * 1000) / mvToAmper);
    }

    inline int GetHeaterCurrentRaw() const {
        return rawHeaterCurrent;
    }

    inline float GetInputCurrent() const {
        float mvToAmper = 90.00;
        return ((RawValueToVoltage(rawInputCurrent) * 1000) / mvToAmper);
    }

    inline int GetInputCurrentRaw() const {
        return rawInputCurrent;
    }

    bool HeaterOvercurentFaultDetected() const;

    bool OvercurrentFaultDetected() const;

    void ResetOvercurrentFault();

    void Update();

private:
    int rawBedVoltage;
    int rawHeaterVoltage;
    int rawHeaterCurrent;
    int rawInputCurrent;
    bool heaterOvercurentFaultDetected;
    bool overcurrentFaultDetected;
    inline float RawValueToVoltage(int rawValue) const {
        return (rawValue * 3.35) / 4096;
    }
};

extern AdvancedPower advancedpower;
