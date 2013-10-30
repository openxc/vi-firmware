#include "util/log.h"
#include "HardwareSerial.h"
#include <stdio.h>
#include <stdarg.h>

using openxc::util::log::debugNoNewline;

extern HardwareSerial Serial2;

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    Serial2.print(buffer);

    va_end(args);
#endif // __DEBUG__
}

void openxc::util::log::initialize() {
    Serial2.begin(115200);
}
