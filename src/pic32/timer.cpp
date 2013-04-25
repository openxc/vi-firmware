#include "timer.h"
#include "WProgram.h"

void openxc::time::delayMs(int delayInMs) {
    delay(delayInMs);
}

unsigned long openxc::time::systemTimeMs() {
    return millis();
}

void openxc::time::initializeTimers() { }
