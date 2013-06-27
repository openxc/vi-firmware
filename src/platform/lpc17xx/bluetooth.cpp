#include "bluetooth.h"
#include "util/log.h"
#include "gpio.h"

#define BLUETOOTH_ENABLE_PORT 0
#define BLUETOOTH_ENABLE_PIN 17

namespace gpio = openxc::gpio;

using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::gpio::GpioValue;

void setBluetoothStatus(bool enabled) {
    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    gpio::setValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            enabled ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
}

void openxc::bluetooth::initialize() {
    debug("Initializing Bluetooth...");
    // be aware that setting the direction here will default it to the off
    // state, so the Bluetooth module will go *off* and then back *on*
    gpio::setDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);
    setBluetoothStatus(true);
    debug("Done.");
}

void openxc::bluetooth::deinitialize() {
    setBluetoothStatus(false);
}
