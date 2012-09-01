#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>

void initializeLogging();

void debug(const char* format, ...);

#endif // _LOG_H_

#ifdef __cplusplus
}
#endif
