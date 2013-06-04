#include <stdlib.h>
#include "serialutil.h"
#include "usbutil.h"
#include "ethernetutil.h"
#include "listener.h"
#include "signals.h"
#include "log.h"
#include "lights.h"
#include "timer.h"
#include "bluetooth.h"
#include "power.h"
#include "platform.h"
#include <stdlib.h>

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_IN_ENDPOINT 1
#define DATA_OUT_ENDPOINT 2

extern void reset();
extern void setup();
extern void loop();

const char* VERSION = "3.2.1";

SerialDevice SERIAL_DEVICE;
EthernetDevice ETHERNET_DEVICE;

UsbDevice USB_DEVICE = {
    DATA_IN_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES,
    DATA_OUT_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Listener listener = {&USB_DEVICE,
#ifdef __USE_UART__
    &SERIAL_DEVICE,
#else
    NULL,
#endif // __USE_UART__
#ifdef __USE_ETHERNET__
    &ETHERNET_DEVICE
#endif // __USE_ETHERNET__
};

/* Public: Update the color and status of a board's light that shows the output
 * interface status. This function is intended to be called each time through
 * the main program loop.
 */
void updateInterfaceLight() {
    if(bluetoothConnected()) {
        enable(LIGHT_B, COLORS.blue);
    } else if(USB_DEVICE.configured) {
        enable(LIGHT_B, COLORS.green);
    } else {
        disable(LIGHT_B);
    }
}

int main(void) {
    initializePlatform();
    initializeLogging();
    initializeTimers();
    initializePower();
    initializeUsb(listener.usb);
    initializeSerial(listener.serial);
    initializeEthernet(listener.ethernet);
    initializeLights();
    initializeBluetooth();

    debug("Initializing as %s", getMessageSet());
    setup();

    for (;;) {
        loop();
        processListenerQueues(&listener);
        updateInterfaceLight();
        updatePower();
    }

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

/* Private: Handle an incoming USB control request.
 *
 * There are two accepted control requests:
 *
 *  - VERSION_CONTROL_COMMAND - return the version of the firmware as a string,
 *      including the vehicle it is built to translate.
 *  - RESET_CONTROL_COMMAND - reset the device.
 *
 *  TODO This function is defined in main.cpp because it needs to reference the
 *  version and message set, which aren't declared in any header files at the
 *  moment. Ripe for refactoring!
 */
bool handleControlRequest(uint8_t request) {
    switch(request) {
    case VERSION_CONTROL_COMMAND:
    {
        char combinedVersion[strlen(VERSION) +
                strlen(getMessageSet()) + 4];
        sprintf(combinedVersion, "%s (%s)", VERSION, getMessageSet());
        debug("Version: %s", combinedVersion);

        sendControlMessage((uint8_t*)combinedVersion, strlen(combinedVersion));
        return true;
    }
    case RESET_CONTROL_COMMAND:
        debug("Resetting...");
        reset();
        return true;
    default:
        return false;
    }
}

#ifdef __cplusplus
}
#endif
