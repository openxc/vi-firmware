#include "log.h"

#ifdef CHIPKIT
#include "WProgram.h"
#endif // CHIPKIT

#define MAX_LOG_LINE_LENGTH 128

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
    printf(format, args);
#endif // CHIPKIT
    va_end(args);
}
