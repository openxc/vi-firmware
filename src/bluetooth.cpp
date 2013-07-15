#include "bluetooth.h"
#include "bluetooth_platforms.h"
#include "interface/uart.h"
#include "util/log.h"
#include "interface/uart.h"
#include "atcommander.h"
#include "util/timer.h"
#include "gpio.h"

#define BLUETOOTH_DEVICE_NAME "OpenXC-VI"

namespace gpio = openxc::gpio;
namespace uart = openxc::interface::uart;

using openxc::interface::uart::UartDevice;
using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::util::time::delayMs;
using openxc::util::log::debugNoNewline;

extern const AtCommanderPlatform AT_PLATFORM_RN42;

void changeBaudRate(void* device, int baud) {
    uart::changeBaudRate((UartDevice*)device, baud);
}

int readByte(void* device) {
    return uart::readByte((UartDevice*)device);
}

void writeByte(void* device, uint8_t byte) {
    uart::writeByte((UartDevice*)device, byte);
}

void openxc::bluetooth::configureExternalModule(UartDevice* device) {
    AtCommanderConfig config = {AT_PLATFORM_RN42};

    config.baud_rate_initializer = changeBaudRate;
    config.device = device;
    config.write_function = writeByte;
    config.read_function = readByte;
    config.delay_function = delayMs;
    config.log_function = debugNoNewline;

    // we most likely just power cycled the RN-42 to make sure it was on, so
    // wait for it to boot up
    delayMs(1000);
    if(at_commander_set_baud(&config, device->baudRate)) {
        debug("Successfully set baud rate");
        if(at_commander_set_name(&config, BLUETOOTH_DEVICE_NAME, true)) {
            debug("Successfully set Bluetooth device name");
        } else {
            debug("Unable to set Bluetooth device name - rebooting anyway");
        }
        at_commander_reboot(&config);
    } else {
        debug("Unable to set baud rate of attached UART device");
    }
}

void setStatus(bool enabled) {
#ifdef BLUETOOTH_ENABLE_SUPPORT
    enabled = BLUETOOTH_ENABLE_PIN_POLARITY ? enabled : !enabled;
    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    gpio::setValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            enabled ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
#endif
}

void openxc::bluetooth::initialize(UartDevice* device) {
#ifdef BLUETOOTH_SUPPORT
    debug("Initializing Bluetooth...");

#ifdef BLUETOOTH_ENABLE_SUPPORT
    // be aware that setting the direction here will default it to the off
    // state, so the Bluetooth module will go *off* and then back *on*
    gpio::setDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            GPIO_DIRECTION_OUTPUT);
    setStatus(true);
#endif

    configureExternalModule(device);
    // re-init to flush any junk in the buffer
    uart::initializeCommon(device);

    debug("Done.");
#endif
}

void openxc::bluetooth::deinitialize() {
    setStatus(false);
}
