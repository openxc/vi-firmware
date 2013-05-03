#include "bluetooth.h"
#include "log.h"
#include "gpio.h"

#define BLUETOOTH_ENABLE_PORT 0
#define BLUETOOTH_ENABLE_PIN 17

void setBluetoothStatus(bool enabled) {
    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    setGpioValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN, enabled);
}

void initializeBluetooth() {
    debug("Initializing Bluetooth...");
    setGpioDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);
    setBluetoothStatus(true);
    debug("Done.");
}
