#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOG_LINE_LENGTH 120

void initializeLogging();

void debug(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // _LOG_H_
