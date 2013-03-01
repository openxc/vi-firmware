#include "bluetooth.h"
#include "gpio.h"

#define BLUETOOTH_ENABLE_PORT 0
#define BLUETOOTH_ENABLE_PIN 17

#define BLUETOOTH_STATUS_PORT 0
#define BLUETOOTH_STATUS_PIN 18

bool bluetoothConnected() {
    return getGpioValue(BLUETOOTH_STATUS_PORT,
            BLUETOOTH_STATUS_PIN) != GPIO_VALUE_LOW;
}

void setBluetoothStatus(bool enabled) {
    setGpioValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN, enabled);
}

void initializeBluetooth() {
    setGpioDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);
    setGpioDirection(BLUETOOTH_STATUS_PORT, BLUETOOTH_STATUS_PIN,
            GPIO_DIRECTION_INPUT);
}
