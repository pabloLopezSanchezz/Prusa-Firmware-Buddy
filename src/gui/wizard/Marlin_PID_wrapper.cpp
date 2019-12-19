// Marlin_PID_wrapper.c

#include "../Marlin/src/module/temperature.h"

extern "C" {

//Kp is not scaled
float get_Kp_Bed() {
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    return 0;
#else //(PRINTER_TYPE != PRINTER_PRUSA_XL)
    return Temperature::temp_bed.pid.Kp;
#endif
}

//Ki is scaled
float get_Ki_Bed() {
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    return 0;
#else //(PRINTER_TYPE != PRINTER_PRUSA_XL)
    return unscalePID_i(Temperature::temp_bed.pid.Ki);
#endif
}

//Kd is scaled
float get_Kd_Bed() {
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    return 0;
#else //(PRINTER_TYPE != PRINTER_PRUSA_XL)
    return unscalePID_d(Temperature::temp_bed.pid.Kd);
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
