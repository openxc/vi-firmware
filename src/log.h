#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>
#include <stdio.h>

#define MAX_LOG_LINE_LENGTH 120

void initializeLogging();

void debug(const char* format, ...);

#endif // _LOG_H_
