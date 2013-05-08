#include "power.h"
#include "log.h"

void initializePower() {
}

void updatePower() {
}

void enterLowPowerMode() {

    /*    This function will invoke the
     *    CPU power save sleep mode.
     *
     *    Bus traffic will wake the CPU and cause
     *     execution to vector to the CAN module's ISR.
     *    The ISR will detect the event as a wakeup and call
     *    the wake-up handler, which simply performs a software reset.
     *
     *    TODO: This function could be a centralized location for
     *     putting peripherals in low power or OFF states. Right now that is
     *  done by the caller (cantranslator.cpp).
     *    This is easy to do via direct manipulation of the peripheral control
     *    registers (SFRs), but skips over the peripheral libraries.
     *    Using the existing C++ libraries here isn't always possible, however,
     *  since they can use overloaded functions, which gcc won't allow.
     */

    debug("Going to low power mode");

    // invoke CPU power save sleep mode
    PowerSaveSleep();

    /* The only peripheral configured with wake-up events should be the
     * CAN1 module. That event will trigger the CAN1 ISR, which will lead
     * directly to a software reset. Code execution should therefore never
     * reach this point. Nevertheless, a software reset would be prudent here.
     */

    SoftReset();

    return;

}

void handleWake() {

    SoftReset();

}
