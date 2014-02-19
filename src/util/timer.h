#ifndef __TIMER_H__
#define __TIMER_H__

namespace openxc {
namespace util {
namespace time {

/* Public: A frequency counting clock.
 *
 * frequency - the clock freuquency in Hz.
 * lastTime - the last time (in milliseconds since startup) that the clock
 *      ticked.
 */
typedef struct {
    int frequency;
    unsigned long lastTick;
    unsigned long (*timeFunction)();
} FrequencyClock;

/* Public: Initialize a FrequencyClock structure back to a fresh start - never
 * ticked, default time function, no set frequency.
 *
 * clock - The clock to initialize.
 */
void initializeClock(FrequencyClock* clock);

/* Public:  Determine if the clock should tick, according to its frequency and
 * last tick time.
 *
 * If the frequency is 0 or the clock is NULL, will return true. The first call
 * to this function always returns true if staggered start is disabled.
 *
 * stagger - If true, adds a random offset to the starting time (between 0 and 1
 *      full period), which is useful if you are initializing multiple clocks
 *      and you want to stagger their ticks. This only applies to the first tick
 *      of the clock.
 *
 * Return true if the clock should tick.
 */
bool shouldTick(FrequencyClock* clock, bool stagger);

/* Public:  The same as shouldTick(FrequencyClock, bool), but staggered start is
 *      off.
 */
bool shouldTick(FrequencyClock* clock);

/* Public: Delay execution by the given number of milliseconds.
 */
void delayMs(unsigned long delayInMs);

/* Public: Return the current system time in milliseconds.
 */
unsigned long systemTimeMs();

/* Public: Perform any one-time initialization required to use system times,
 * including those for system time and the delayMs function.
 */
void initialize();

/* Public: Return the system time in milliseconds when the code started to run.
 */
unsigned long startupTimeMs();

/* Public: Return the uptime of the code in milliseconds. */
unsigned long uptimeMs();

} // namespace time
} // namespace util
} // namespace openxc

#endif // __TIMER__H__
