#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include "debug_frmwrk.h"
}

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    _printf(buffer);

    va_end(args);
#endif // __DEBUG__
}

void openxc::util::log::initializeLogging() {
    debug_frmwrk_init();
}
