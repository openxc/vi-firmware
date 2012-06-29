#include "buffers.h"
#include "strutil.h"
#include "WProgram.h"

void resetBuffer(char* buffer, int* bufferIndex, const int bufferSize) {
    *bufferIndex = 0;
    memset(buffer, 0, bufferSize);
}

void processBuffer(char* buffer, int* bufferIndex, const int bufferSize,
        bool (*callback)(char*)) {
    if(callback(buffer)) {
        resetBuffer(buffer, bufferIndex, bufferSize);
    } else if(*bufferIndex >= 4) {
        Serial.println("Incoming write is too long");
        resetBuffer(buffer, bufferIndex, bufferSize);
    } else if(strnchr(buffer, bufferSize, NULL) != NULL) {
        Serial.println("Incoming buffered write is corrupted -- "
                "clearing buffer");
        resetBuffer(buffer, bufferIndex, bufferSize);
    }
}
