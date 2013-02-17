#include "power.h"

#define SLEEP_MODE_ENABLE_BIT 4

void initializePower() {
}

void updatePower() {
}

void enterLowPowerMode() {
    // When WAIT instruction is executed, go into SLEEP mode
    OSCCONSET = (1 << SLEEP_MODE_ENABLE_BIT);
    asm("wait");
    // TODO if we wake up, do we resume with the PC right here? there's an RCON
    // register with bits to indicate if we woke up from sleep. we'll need to
    // re-run setup, potentially.
    //C1CONSET / C1CONbits.x
}
