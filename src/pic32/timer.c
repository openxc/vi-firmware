#include "timer.h"
#include "WProgram.h"

void delayMs(int delayInMs) {
    delay(delayInMs);
}

unsigned long systemTimeMs() {
    return millis();
}

void initializeTimers() { }
