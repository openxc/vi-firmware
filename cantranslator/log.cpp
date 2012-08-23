#include "log.h"
#include <stdio.h>

#ifdef CHIPKIT
#include "WProgram.h"
#endif // CHIPKIT

void log(const char* message) {
#ifdef CHIPKIT
    Serial.println(message);
#else
    printf("%s\n", message);
#endif // CHIPKIT
}
