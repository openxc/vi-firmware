#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

namespace openxc {
namespace bluetooth {

/* Public: Perform any initialization required to control Bluetooth. */
void initializeBluetooth();

/* Public: Shut down the bluetooth peripheral (save power). */
void deinitializeBluetooth();

} // namespace bluetooth
} // namespace openxc

#endif // _BLUETOOTH_H_
