#ifndef MEM_TRACKING__H
#define MEM_TRACKING__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define malloc(x) custom_malloc(x, __FILE__, __LINE__)
#define free(x) custom_free(x, __FILE__, __LINE__)


void* custom_malloc(size_t sz, const char* file, int line);
void custom_free(void* ptr, const char* file, int line);

extern unsigned long totalBytes;
extern unsigned long totalMallocs;

#ifdef __cplusplus
}
#endif

#endif