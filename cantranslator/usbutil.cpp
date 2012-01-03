#include "usbutil.h"

USB_HANDLE USB_INPUT_HANDLE = 0;

char SEND_BUFFER[SEND_BUFFER_SIZE];

void sendMessage(USBDevice* usbDevice, uint8_t* message, int messageSize) {
#ifdef DEBUG
    Serial.print("sending message: ");
    Serial.println((char*)message);
#endif

    // Make sure the USB write is 100% complete before messing with this buffer
    // after we copy the message into it - the Microchip library doesn't copy
    // the data to its own internal buffer. See #171 for background on this
    // issue.
    while(usbDevice->HandleBusy(USB_INPUT_HANDLE));
    strncpy(SEND_BUFFER, (char*)message, messageSize);

    int nextByteIndex = 0;
    while(nextByteIndex < messageSize) {
        while(usbDevice->HandleBusy(USB_INPUT_HANDLE));
        int bytesToTransfer = min(USB_PACKET_SIZE,
                messageSize - nextByteIndex);
        USB_INPUT_HANDLE = usbDevice->GenWrite(DATA_ENDPOINT,
                (uint8_t*)(SEND_BUFFER + nextByteIndex), bytesToTransfer);
        nextByteIndex += bytesToTransfer;
    }
}

void initializeUsb(USBDevice* usbDevice) {
    Serial.print("Initializing USB...  ");
    usbDevice->InitializeSystem(true);
    while(usbDevice->GetDeviceState() < CONFIGURED_STATE);
    Serial.println("Done.");
}
