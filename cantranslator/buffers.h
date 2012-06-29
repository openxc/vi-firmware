#ifndef _BUFFERS_H_
#define _BUFFERS_H_

void resetBuffer(char* buffer, int* bufferIndex, const int bufferSize);

void processBuffer(char* buffer, int* bufferIndex, const int bufferSize,
        bool (*callback)(char*));

#endif // _BUFFERS_H_
