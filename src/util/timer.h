#ifndef __TIMER_H__
#define __TIMER_H__

namespace openxc {
namespace util {
namespace time {

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

} // namespace time
} // namespace util
} // namespace openxc

#endif // __TIMER__H__
