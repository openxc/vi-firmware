#include "log.h"
#include <stdio.h>

void debug(const char* format, ...) {
#ifdef __DEBUG__
    vprintf(format, args);
#endif // __DEBUG__
}

void initializeLogging() { }
