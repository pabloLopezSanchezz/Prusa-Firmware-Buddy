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
        return ((RawValueToVoltage(rawHeaterCurrent) / 2.20f) * 2);
    }

    inline int GetHeaterCurrentRaw() const {
        return rawHeaterCurrent;
    }

    inline float GetInputCurrent() const {
        return ((2.4f / RawValueToVoltage(rawInputCurrent)) * 0.9f);
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
