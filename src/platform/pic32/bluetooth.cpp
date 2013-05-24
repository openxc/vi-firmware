#include "bluetooth.h"
#include "gpio.h"
#include "util/log.h"

using openxc::gpio::GpioValue;
using openxc::gpio::getGpioValue;

#if defined(FLEETCARMA)

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
    GpioValue value = BLUETOOTH_ENABLE_PIN_POLARITY ? enabled : !enabled;

    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    setGpioValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN, value);
#endif
}

void openxc::bluetooth::initialize() {
#ifdef BLUETOOTH_SUPPORT
    debug("Initializing Bluetooth...");

    // initialize bluetooth enable and status pins
    setGpioDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);
    setGpioDirection(BLUETOOTH_STATUS_PORT, BLUETOOTH_STATUS_PIN,
            GPIO_DIRECTION_INPUT);

    setBluetoothStatus(true);

    debug("Done.");
#endif
}

void openxc::bluetooth::deinitialize() {
    setBluetoothStatus(false);
}
