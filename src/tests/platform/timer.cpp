#include "util/timer.h"

void openxc::util::time::delayMs(unsigned long delayInMs) { }

unsigned long FAKE_TIME = 1000;

unsigned long openxc::util::time::systemTimeMs() {
    return FAKE_TIME;
}

void openxc::util::time::initialize() { }
