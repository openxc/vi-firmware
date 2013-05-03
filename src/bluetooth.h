#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* Public: Shut down the bluetooth peripheral (save power). */
void deinitializeBluetooth();


#ifdef __cplusplus
}
#endif

#endif // _BLUETOOTH_H_
