#ifndef _STATISTICS_H_
#define _STATISTICS_H_

namespace openxc {
namespace util {
namespace statistics {

/* Public: A helper struct for calculating basic numerical statistics.
 *
 * min - holds the minimum value seen so far.
 * max - holds the maximum value seen so far.
 * movingAverage - the exponential moving average seen so far.
 * alpha - controls the size of the moving average - alpha == 1 / N where N is
 *      the window size. The default is .1.
 */
typedef struct {
    float min;
    float max;
    float movingAverage;
    float alpha;
} Statistic;

/* Public: Initialize a new Statistic.
 *
 * stat - the Statistic to initialize.
 */
void initialize(Statistic* stat);

/* Public: Update the statistic with a new observed value.
 *
 * This will update the min, max and movingAverage fields on the struct - it's
 * not great encapsulation, but you can just access those fields directly after
 * calling this function.
 *
 * stat - the Statistic object to update.
 * newValue - the newly observed value.
 */
void update(Statistic* stat, float newValue);

} // namespace statistics
} // namespace util
} // namespace openxc

#endif // _STATISTICS_H_
