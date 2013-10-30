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
    bool started;
} FrequencyClock;

void initializeClock(FrequencyClock* clock);

/* Public:  Determine if the clock should tick, according to its frequency and
 * last tick time.
 *
 * If the frequency is 0 or the clock is NULL, will return true. The first call
 * to this function always returns true.
 *
 * Return true if the clock should tick.
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
