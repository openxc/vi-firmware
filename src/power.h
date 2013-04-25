#ifndef _POWER_H_
#define _POWER_H_

namespace openxc {
namespace power {

/* Public: Initialize any systems replated to power for the microcontroller or
 * connected peripherals.
 */
void initializePower();

// TODO kill this method
void updatePower();

/* Public: Shut down all peripherals, set up interrupts to wake on CAN activity
 * and put the microcontroller into a low power mode.
 *
 * This mode should be suitable for remaining attached to the 12v line of a
 * vehicle and must not drain the battery. If the program continues after wakeup
 * and returns from this function, all perpherals should be put back into the
 * active state. An alternative is to reset the entire system after wakeup, in
 * which case this function never returns.
 */
void enterLowPowerMode();

} // namespace power
} // namespace openxc

#endif // _POWER_H_
