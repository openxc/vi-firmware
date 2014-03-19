#ifndef __TIMER_H__
#define __TIMER_H__

namespace openxc {
namespace util {
namespace time {

typedef unsigned long (*TimeFunction)();

/* Public: A frequency counting clock.
 *
 * frequency - the clock freuquency in Hz.
 * lastTime - the last time (in milliseconds since startup) that the clock
 *      ticked.
 */
typedef struct {
    float frequency;
    unsigned long lastTick;
    TimeFunction timeFunction;
} FrequencyClock;

/* Public: Initialize a FrequencyClock structure back to a fresh start - never
 * ticked, default time function, no set frequency.
 *
 * clock - The clock to initialize.
 */
void initializeClock(FrequencyClock* clock);

/* Public: Determine if the clock should tick, according to its frequency and
 * last tick time, and tick it if it needs it!
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
bool conditionalTick(FrequencyClock* clock, bool stagger);

/* Public:  The same as conditionalTick(FrequencyClock, bool), but staggered start is
 *      off.
 */
bool conditionalTick(FrequencyClock* clock);

/* Public: Determine if the clock's tick timer has elapsed and it should tick.
 * Does *not* actually tick the clock.
 *
 * stagger - If true, will set the clock back a random amount if it hasn't
 *      started, so it doesn't start right away with the first call to
 *      conditionalTick(...). Still, it doesn't actually tick it.
 */
bool elapsed(FrequencyClock* clock, bool stagger);

/* Public: Force the clock to tick, regardless of it its time has actually
 * elapsed.
 */
void tick(FrequencyClock* clock);

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
