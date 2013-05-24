#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif // __DEBUG__
}

void openxc::util::log::initialize() { }
