#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

void delayMs(int delayInMs);

/* Public: Return the current system time in milliseconds.
 */
unsigned long systemTimeMs();

void initializeTimers();

#ifdef __cplusplus
}
#endif

#endif // __TIMER__H__
