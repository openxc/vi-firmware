#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

namespace openxc {
namespace bluetooth {

/* Public: Check the connection status of a Bluetooth adapter.
 *
 * Returns true if Bluetooth is connected to master device.
 */
bool bluetoothConnected();

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
