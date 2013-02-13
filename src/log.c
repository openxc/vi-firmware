#include "log.h"
#include "debug_frmwrk.h"
#include <stdio.h>
#include <stdarg.h>

void debug(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    debugNoNewline(format, args);
    debugNoNewline("\r\n");

    va_end(args);
#endif // __DEBUG__
}
