#include "log.h"
#include "debug_frmwrk.h"
#include <stdio.h>
#include <stdarg.h>

void debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    _printf(buffer);

    va_end(args);
#endif // __DEBUG__
}

void initializeLogging() {
    debug_frmwrk_init();
}
