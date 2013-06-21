#include "bluetooth.h"
#include "gpio.h"
#include "util/log.h"

namespace gpio = openxc::gpio;

using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;

#ifdef FLEETCARMA

    #define BLUETOOTH_SUPPORT

    #define BLUETOOTH_ENABLE_PIN_POLARITY 1 // drive high == power on
    #define BLUETOOTH_ENABLE_PORT 0
    #define BLUETOOTH_ENABLE_PIN 32 // PORTE BIT5 (RE5)

    // no other PIC32 boards supported right now have an enable pin for
    // Bluetooth - using the Sparkfun BlueSMiRF module, you can't control the
    // status via GPIO.

#endif

void setBluetoothStatus(bool enabled) {
#ifdef BLUETOOTH_SUPPORT
    enabled = BLUETOOTH_ENABLE_PIN_POLARITY ? enabled : !enabled;
    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    gpio::setValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            enabled ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
#endif
}

void openxc::bluetooth::initialize() {
#ifdef BLUETOOTH_SUPPORT
    debug("Initializing Bluetooth...");

    // initialize bluetooth enable and status pins
    gpio::setDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);

    setBluetoothStatus(true);

    debug("Done.");
#endif
}

void openxc::bluetooth::deinitialize() {
    setBluetoothStatus(false);
}
