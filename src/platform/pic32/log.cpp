#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>
#include "WProgram.h"

extern HardwareSerial Serial2;

void openxc::util::log::initialize() {
    Serial2.begin(115200);
}

void openxc::util::log::debugUart(const char* message) {
    Serial2.print(message);
}
