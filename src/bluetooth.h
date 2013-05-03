#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public: Enable or disable a Bluetooth module.
 *
 * status - if true, Bluetooth will be enabled, otherwise it will be disabled.
 */
void setBluetoothStatus(bool status);

/* Public: Perform any initialization required to control Bluetooth. */
void initializeBluetooth();

#ifdef __cplusplus
}
#endif

#endif // _BLUETOOTH_H_
