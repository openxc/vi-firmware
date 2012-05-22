#include "usbutil.h"

USB_HANDLE USB_INPUT_HANDLE = 0;

char SEND_BUFFER[ENDPOINT_SIZE];

void sendMessage(CanUsbDevice* usbDevice, uint8_t* message, int messageSize) {
#ifdef DEBUG
    Serial.print("sending message: ");
    Serial.println((char*)message);
#endif

    // Make sure the USB write is 100% complete before messing with this buffer
    // after we copy the message into it - the Microchip library doesn't copy
    // the data to its own internal buffer. See #171 for background on this
    // issue.
    while(usbDevice->device.HandleBusy(USB_INPUT_HANDLE));
    strncpy(SEND_BUFFER, (char*)message, messageSize);
    SEND_BUFFER[messageSize] = '\n';
    messageSize += 1;

    int nextByteIndex = 0;
    while(nextByteIndex < messageSize) {
        while(usbDevice->device.HandleBusy(USB_INPUT_HANDLE));
        int bytesToTransfer = min(USB_PACKET_SIZE,
                messageSize - nextByteIndex);
        USB_INPUT_HANDLE = usbDevice->device.GenWrite(usbDevice->endpoint,
                (uint8_t*)(SEND_BUFFER + nextByteIndex), bytesToTransfer);
        nextByteIndex += bytesToTransfer;
    }
}

void initializeUsb(CanUsbDevice* usbDevice) {
    Serial.print("Initializing USB...  ");
    usbDevice->device.InitializeSystem(true);
    while(usbDevice->device.GetDeviceState() < CONFIGURED_STATE);
    Serial.println("Done.");
}
