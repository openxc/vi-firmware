#include "bluetooth.h"

#include "bluetooth_platforms.h"
#include "util/log.h"
#include "atcommander.h"
#include "util/timer.h"
#include "gpio.h"
#include "config.h"
#include <string.h>

#define BLUETOOTH_DEVICE_NAME "OpenXC-VI"
#define BLUETOOTH_SLAVE_MODE 0
#define BLUETOOTH_PAIRING_MODE 6
#define BLUETOOTH_AUTO_MASTER_MODE 3

namespace gpio = openxc::gpio;
namespace uart = openxc::interface::uart;

using openxc::interface::uart::UartDevice;
using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::config::getConfiguration;
using openxc::util::time::delayMs;
using openxc::util::log::debug;

extern const AtCommanderPlatform AT_PLATFORM_RN42;

static void changeBaudRate(void* device, int baud) {
    uart::changeBaudRate((UartDevice*)device, baud);
}

static int readByte(void* device) {
    return uart::readByte((UartDevice*)device);
}

static void writeByte(void* device, uint8_t byte) {
    uart::writeByte((UartDevice*)device, byte);
}

void openxc::bluetooth::configureExternalModule(UartDevice* device) {
#ifdef CHIPKIT
    if(!uart::connected(device)) {
        debug("UART is physically disabled on a chipKIT - not attempting to configure Bluetooth");
        return;
    }
#endif

    AtCommanderConfig config = {AT_PLATFORM_RN42};

    config.baud_rate_initializer = changeBaudRate;
    config.device = device;
    config.write_function = writeByte;
    config.read_function = readByte;
    config.delay_function = delayMs;
    config.log_function = debug;

    // we most likely just power cycled the RN-42 to make sure it was on, so
    // wait for it to boot up
    delayMs(1000);
    if(at_commander_set_baud(&config, device->baudRate)) {
        debug("Successfully set baud rate");
        if(at_commander_set_name(&config, BLUETOOTH_DEVICE_NAME, true)) {
            debug("Successfully set Bluetooth device name");
        } else {
            debug("Unable to set Bluetooth device name");
        }

        if(at_commander_get_device_id(&config, device->deviceId,
                    sizeof(device->deviceId)) > 0) {
            debug("Bluetooth MAC is %s", device->deviceId);
        } else {
            debug("Unable to get Bluetooth MAC");
            device->deviceId[0] = '\0';
        }

        if(at_commander_set_configuration_timer(&config, 0)) {
            debug("Successfully disabled remote Bluetooth configuration");
        } else {
            debug("Unable to disable remote Bluetooth configuration");
        }

        AtCommand pinCommand = {
            request_format: "SP,%s\r",
            expected_response: "AOK",
            error_response: "ERR"
        };
	
        if(at_commander_set(&config, &pinCommand, getConfiguration()->bluetoothPin)) {
            debug("Changed Bluetooth Pairing Pin to %s.", getConfiguration()->bluetoothPin);
        } else {
            debug("Unable to change Bluetooth Pairing Pin.");
        }

        AtCommand inquiryCommand = {
            request_format: "SI,%s\r",
            expected_response: "AOK",
            error_response: "ERR"
        };

        if(at_commander_set(&config, &inquiryCommand, "0200")) {
            debug("Changed Bluetooth inquiry window to 0200");
        } else {
            debug("Unable to change Bluetooth inquiry window.");
        }

        AtCommand pagingCommand = {
            request_format: "SJ,%s\r",
            expected_response: "AOK",
            error_response: "ERR"
        };

        if(at_commander_set(&config, &pagingCommand, "0200")) {
            debug("Changed Bluetooth page scan window to 0200");
        } else {
            debug("Unable to change Bluetooth page scan window.");
        }

        AtCommand firmwareVersionCommand = {
            request_format: "V\r",
            expected_response: NULL,
            error_response: "ERR"
        };

        char versionString[64];
        if(!at_commander_get(&config, &firmwareVersionCommand, versionString, sizeof(versionString))) {
            debug("Unable to determine Bluetoothe module firmware version");
        } else {
            debug("Bluetooth module is running firmware %s", versionString);
            AtCommand modeCommand = {
                request_format: "SM,%d\r",
                expected_response: "AOK",
                error_response: "ERR"
            };

            int desiredMode = BLUETOOTH_SLAVE_MODE;
            if(strstr(versionString, "6.") != NULL) {
                debug("Bluetooth device is on 6.x firmware - switching to pairing mode");
                desiredMode = BLUETOOTH_PAIRING_MODE;
            } else {
                debug("Bluetooth device is on 4.x firmware - switching to slave mode");
            }

            if(!at_commander_set(&config, &modeCommand, desiredMode)) {
                debug("Unable to change Bluetooth device mode");
            }
        }

        at_commander_reboot(&config);
    } else {
        debug("Unable to set baud rate of attached UART device");
    }
}

// Only want to set the directly once because it flips the power on/off.
#ifdef BLUETOOTH_ENABLE_SUPPORT
static void setIoDirection() {
    static bool directionSet = false;
    if(!directionSet) {
        // be aware that setting the direction here will default it to the off
        // state, so the Bluetooth module will go *off* and then back *on*
        gpio::setDirection(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
                GPIO_DIRECTION_OUTPUT);
        directionSet = true;
    }
}
#endif

static void setStatus(bool enabled) {
// setStatus will only run if BlueTooth support is enabled
#ifdef BLUETOOTH_ENABLE_SUPPORT
    enabled = BLUETOOTH_ENABLE_PIN_POLARITY ? enabled : !enabled;
    debug("Turning Bluetooth %s", enabled ? "on" : "off");
    setIoDirection();
    gpio::setValue(BLUETOOTH_ENABLE_PORT, BLUETOOTH_ENABLE_PIN,
            enabled ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
#endif
}

void openxc::bluetooth::start(UartDevice* device) {
#ifdef BLUETOOTH_SUPPORT
    debug("Starting Bluetooth...");
    setStatus(true);

    strcpy(device->deviceId, "Unknown");
    configureExternalModule(device);
    // re-init to flush any junk in the buffer
    uart::initializeCommon(device);

    debug("Done.");
#endif
}

void openxc::bluetooth::initialize(UartDevice* device) {
// Initializing Bluetooth in disabled state for UART device
#ifdef BLUETOOTH_SUPPORT
    debug("Initializing Bluetooth in disabled state...");
    setStatus(false);
#endif
}

void openxc::bluetooth::deinitialize() {
    setStatus(false);
}
