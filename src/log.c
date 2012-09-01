#include "log.h"
#include "debug_frmwrk.h"
#include <stdio.h>

void debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
#ifdef CHIPKIT
    char* buffer = (char*) malloc(MAX_LOG_LINE_LENGTH);
    if(buffer != NULL) {
        vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);
        Serial.println(buffer);
        free(buffer);
    }
#else
    _printf(format, args);
#endif // CHIPKIT
    va_end(args);
}

void initializeLogging() {
#ifdef CHIPKIT
    Serial.begin(115200);
#else
    debug_frmwrk_init();
#endif // CHIPKIT
}
