#include "power_spy.h"

int watchdogTime = 0;

int openxc::power::spy::getWatchdogTime() {
    return watchdogTime;
}

void openxc::power::initialize() { }

void openxc::power::handleWake() { }

void openxc::power::suspend() { }

void openxc::power::enableWatchdogTimer(int microseconds) {
    watchdogTime = microseconds;
}

void openxc::power::disableWatchdogTimer() {
    watchdogTime = 0;
}

void openxc::power::feedWatchdog() { }
