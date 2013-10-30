#include "util/timer.h"
#include "WProgram.h"

void openxc::util::time::delayMs(unsigned long delayInMs) {
    delay(delayInMs);
}

unsigned long openxc::util::time::systemTimeMs() {
    return millis();
}

void openxc::util::time::initialize() { }
