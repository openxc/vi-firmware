#include "serialutil.h"
#include "usbutil.h"
#include "listener.h"
#include "signals.h"
#include "log.h"
#include <stdlib.h>

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_ENDPOINT 1

extern void reset();
extern void setup();
extern void loop();

#ifdef __PIC32__
extern boolean usbCallback(USB_EVENT, void*, word);
#endif // __PIC32__

const char* VERSION = "2.0-pre";

#ifdef __PIC32__
SerialDevice SERIAL_DEVICE = {&Serial1};
#else
SerialDevice SERIAL_DEVICE;
#endif // __PIC32__

UsbDevice USB_DEVICE = {
#ifdef __PIC32__
    USBDevice(usbCallback),
#endif // __PIC32__
    DATA_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Listener listener = {&USB_DEVICE, &SERIAL_DEVICE};

int main(void) {
#ifdef __PIC32__
    init();
#endif // __PIC32__
    setup();

    for (;;)
        loop();

    return 0;
}

bool handleControlRequest(uint8_t request) {
    switch(request) {
    case VERSION_CONTROL_COMMAND:
    {
        char* combinedVersion = (char*)malloc(strlen(VERSION) +
                strlen(getMessageSet()) + 4);
        sprintf(combinedVersion, "%s (%s)", VERSION, getMessageSet());
        debug("Version: %s\r\n", combinedVersion);

        sendControlMessage((uint8_t*)combinedVersion, strlen(combinedVersion));
        free(combinedVersion);
        return true;
    }
    case RESET_CONTROL_COMMAND:
        debug("Resetting...\r\n");
        reset();
        return true;
    default:
        return false;
    }
}

