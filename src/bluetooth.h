#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

namespace openxc {
namespace bluetooth {

/* Public: Enable or disable a Bluetooth module.
 *
 * status - if true, Bluetooth will be enabled, otherwise it will be disabled.
 */
void setBluetoothStatus(bool status);

/* Public: Perform any initialization required to control Bluetooth. */
void initializeBluetooth();

} // namespace bluetooth
} // namespace openxc

#endif // _BLUETOOTH_H_
