// Marlin_PID_wrapper.c

#include "../Marlin/src/module/temperature.h"

extern "C" {

//Kp is not scaled
float get_Kp_Bed() {
#if ENABLED(PIDTEMPBED)
    return Temperature::temp_bed.pid.Kp;
#else
    return 0;
#endif
}

//Ki is scaled
float get_Ki_Bed() {
#if ENABLED(PIDTEMPBED)
    return unscalePID_i(Temperature::temp_bed.pid.Ki);
#else
    return 0;
#endif
}

//Kd is scaled
float get_Kd_Bed() {
#if ENABLED(PIDTEMPBED)
    return unscalePID_d(Temperature::temp_bed.pid.Kd);
#else
    return 0;
#endif
}

//Kp is not scaled
float get_Kp_Noz() {
    return PID_PARAM(Kp, 0);
}

//Ki is scaled
float get_Ki_Noz() {
    return unscalePID_i(PID_PARAM(Ki, 0));
}

//Kd is scaled
float get_Kd_Noz() {
    return unscalePID_d(PID_PARAM(Kd, 0));
}

} //extern "C"
