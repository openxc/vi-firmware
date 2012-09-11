#include "log.h"
#include <stdio.h>

#ifdef __LPC17XX__
#include "debug_frmwrk.h"
#endif

#ifdef __PIC32__
#include "WProgram.h"
#endif

void debug(const char* format, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, format);

#ifdef __TESTS__
    vprintf(format, args);
#else
    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

#ifdef __PIC32__
    Serial.println(buffer);
#endif // __PIC32__

#ifdef  __LPC17XX__
    _printf(buffer);
#endif // __LPC17XX__

#endif //_TESTS__

    va_end(args);
#endif
}

void initializeLogging() {
#ifdef __PIC32__
    Serial.begin(115200);
#endif // __PIC32__

#ifdef   __LPC17XX__
    debug_frmwrk_init();
#endif // __LPC17XX__
}
