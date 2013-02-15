#include "serialutil.h"
#include "usbutil.h"
#include "ethernetutil.h"
#include "listener.h"
#include "signals.h"
#include "log.h"
#include "timer.h"
#include <stdlib.h>

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_IN_ENDPOINT 1
#define DATA_OUT_ENDPOINT 2

extern void reset();
extern void setup();
extern void loop();

const char* VERSION = "3.0-dev";

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

int main(void) {
#ifdef __PIC32__
    init();
#endif // __PIC32__

    initializeLogging();
    initializeTimers();
    initializeUsb(listener.usb);
    initializeSerial(listener.serial);
    initializeEthernet(listener.ethernet);

    debug("Initializing as %s", getMessageSet());
    setup();

    for (;;) {
        loop();
        processListenerQueues(&listener);
    }

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

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
