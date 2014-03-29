#include "power.h"
#include "util/log.h"
#include <plib.h>

using openxc::util::log::debug;

void openxc::power::initialize() {
}

/* This function will invoke the CPU power save sleep mode.
 *
 * Bus traffic will wake the CPU and cause execution to vector to the CAN
 * module's ISR. The ISR will detect the event as a wakeup and call the wake-up
 * handler, which simply performs a software reset.
 *
 * TODO: This function could be a centralized location for putting peripherals
 * in low power or OFF states. Right now that is done by the caller
 * (vi-firmware.cpp). This is easy to do via direct manipulation of the
 * peripheral control registers (SFRs), but skips over the peripheral libraries.
 * Using the existing C++ libraries here isn't always possible, however, since
 * they can use overloaded functions, which gcc won't allow.
 */
void openxc::power::suspend() {
    debug("Going to low power mode");

    PowerSaveSleep();

    // The only peripheral configured with wake-up events should be the CAN1
    // module. That event will trigger the CAN1 ISR, which will lead directly to
    // a software reset. Code execution should therefore never reach this point.
    // Nevertheless, a software reset would be prudent here.
    SoftReset();
}

void openxc::power::handleWake() {
    SoftReset();
}

void openxc::power::enableWatchdogTimer(int microseconds) {
    // TODO argh, can't change postscaler value from software because it's
    // configured with a #pragma directive in the bootloader. The time for the
    // WDT is permanently set at about 1 second. We could write to the
    // configuration register in flash memory, but we'd want to make sure that
    // only happened once because flash has a 10k write limit.
    EnableWDT();
}

void openxc::power::disableWatchdogTimer() {
    DisableWDT();
}

void openxc::power::feedWatchdog() {
    ClearWDT();
}
