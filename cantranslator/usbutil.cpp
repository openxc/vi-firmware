#include "usbutil.h"

USBDevice USB_DEVICE(usbCallback);
USB_HANDLE USB_INPUT_HANDLE = 0;

char SEND_BUFFER[SEND_BUFFER_SIZE];

void sendMessage(uint8_t* message, int messageSize) {
#ifdef DEBUG
    Serial.print("sending message: ");
    Serial.println((char*)message);
#endif

    // Make sure the USB write is 100% complete before messing with this buffer
    // after we copy the message into it - the Microchip library doesn't copy
    // the data to its own internal buffer. See #171 for background on this
    // issue.
    while(USB_DEVICE.HandleBusy(USB_INPUT_HANDLE));
    strncpy(SEND_BUFFER, (char*)message, messageSize);

    int nextByteIndex = 0;
    while(nextByteIndex < messageSize) {
        while(USB_DEVICE.HandleBusy(USB_INPUT_HANDLE));
        int bytesToTransfer = min(USB_PACKET_SIZE,
                messageSize - nextByteIndex);
        USB_INPUT_HANDLE = USB_DEVICE.GenWrite(DATA_ENDPOINT,
                (uint8_t*)(SEND_BUFFER + nextByteIndex), bytesToTransfer);
        nextByteIndex += bytesToTransfer;
    }
}

void initializeUsb() {
    Serial.print("Initializing USB...  ");
    USB_DEVICE.InitializeSystem(true);
    while(USB_DEVICE.GetDeviceState() < CONFIGURED_STATE);
    Serial.println("Done.");
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    // initial connection up to configure will be handled by the default
    // callback routine.
    USB_DEVICE.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        Serial.println("Event: Configured");
        // Enable DATA_ENDPOINT for input
        USB_DEVICE.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        break;
    default:
        break;
    }
}
