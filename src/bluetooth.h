#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "interface/uart.h"

namespace openxc {
namespace bluetooth {

/* Public: Perform any initialization required to control Bluetooth.
 *
 * The radio will be initialized in a disabled, low to no power state.
 *
 */
void initialize(openxc::interface::uart::UartDevice* device);

/* Public: Start the Bluetooth interface.
 *
 * The radio will being advertising and consuming power.
 */
void start(openxc::interface::uart::UartDevice* device);

/* Public: Shut down the bluetooth peripheral (save power). */
void deinitialize();

/* Public: Configure the baud rate and other parameters on an external Bluetooth
 * module.
 */
void configureExternalModule(openxc::interface::uart::UartDevice* device);

} // namespace bluetooth
} // namespace openxc

#endif // _BLUETOOTH_H_
