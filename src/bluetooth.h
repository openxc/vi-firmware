#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "interface/uart.h"

namespace openxc {
namespace bluetooth {

/* Public: Perform any initialization required to control Bluetooth. */
void initialize(openxc::interface::uart::UartDevice* device);

/* Public: Shut down the bluetooth peripheral (save power). */
void deinitialize();

/* Public: The main entry for the Bluetooth logic, this method should be called
 * each time through the main loop so it can detect changes in Bluetooth
 * connection status.
 */
void loop(openxc::interface::uart::UartDevice* device);

/* Public: Configure the baud rate and other parameters on an external Bluetooth
 * module.
 */
void configureExternalModule(openxc::interface::uart::UartDevice* device);

} // namespace bluetooth
} // namespace openxc

#endif // _BLUETOOTH_H_
