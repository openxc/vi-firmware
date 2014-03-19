#include "power.h"
#include "util/log.h"

#define WATCHDOG_TIMEOUT_MICROSECONDS 10000000

using openxc::util::log::debug;

void openxc::power::initializeCommon() {
    debug("Initializing peripheral power...");
    enableWatchdogTimer(WATCHDOG_TIMEOUT_MICROSECONDS);
}
