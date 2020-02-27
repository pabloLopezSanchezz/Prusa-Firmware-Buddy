#include "loadcell_endstops.h"
#include "../Marlin/src/module/endstops.h"

void endstops_poll() {
    endstops.poll();
}
