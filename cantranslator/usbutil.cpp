#include "usbutil.h"

USBDevice USB_DEVICE(usbCallback);
USB_HANDLE USB_INPUT_HANDLE = 0;

void sendMessage(uint8_t* message, int messageSize) {
    int nextByteIndex = 0;
#ifdef DEBUG
    Serial.print("sending message: ");
    Serial.println((char*)message);
#endif

    while(nextByteIndex < messageSize) {
        while(USB_DEVICE.HandleBusy(USB_INPUT_HANDLE));
        int bytesToTransfer = min(USB_PACKET_SIZE,
                messageSize - nextByteIndex);
        USB_INPUT_HANDLE = USB_DEVICE.GenWrite(DATA_ENDPOINT,
                (uint8_t*)(message + nextByteIndex), bytesToTransfer);
        nextByteIndex += bytesToTransfer;
    }
    while(USB_DEVICE.HandleBusy(USB_INPUT_HANDLE));
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
