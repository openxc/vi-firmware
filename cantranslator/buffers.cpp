#include "buffers.h"
#include "strutil.h"
#include <stdio.h>
#include <string.h>

void resetBuffer(char* buffer, int* bufferIndex, const int bufferSize) {
    *bufferIndex = 0;
    memset(buffer, '\n', bufferSize);
}

void processBuffer(char* buffer, int* bufferIndex, const int bufferSize,
        bool (*callback)(char*)) {
    if(callback(buffer)) {
        resetBuffer(buffer, bufferIndex, bufferSize);
    } else if(*bufferIndex >= bufferSize) {
        printf("Incoming write is too long");
        resetBuffer(buffer, bufferIndex, bufferSize);
    } else if(strnchr(buffer, bufferSize, '\0') != NULL) {
        printf("Incoming buffered write is corrupted -- clearing buffer");
        resetBuffer(buffer, bufferIndex, bufferSize);
    }
}
