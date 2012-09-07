#include "log.h"
#include "debug_frmwrk.h"
#include <stdio.h>

void debug(const char* format, ...) {
    va_list args;
    va_start(args, format);

#ifdef __TESTS__
    vprintf(format, args);
#else
    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

#ifdef CHIPKIT
    Serial.println(buffer);
#endif // CHIPKIT

#ifdef  __LPC17XX__
    _printf(buffer);
#endif // __LPC17XX__

#endif //_TESTS__

    va_end(args);
}

void initializeLogging() {
#ifdef CHIPKIT
    Serial.begin(115200);
#endif // CHIPKIT

#ifdef   __LPC17XX__
    debug_frmwrk_init();
#endif // __LPC17XX__
}
