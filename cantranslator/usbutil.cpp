#include "usbutil.h"

// TODO move this up to cantranslator.pde
USB_HANDLE USB_INPUT_HANDLE = 0;


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
    strncpy(usbDevice->sendBuffer, (char*)message, messageSize);
    usbDevice->sendBuffer[messageSize] = '\n';
    messageSize += 1;

    int nextByteIndex = 0;
    while(nextByteIndex < messageSize) {
        while(usbDevice->device.HandleBusy(USB_INPUT_HANDLE));
        int bytesToTransfer = min(USB_PACKET_SIZE,
                messageSize - nextByteIndex);
        USB_INPUT_HANDLE = usbDevice->device.GenWrite(usbDevice->endpoint,
                (uint8_t*)(usbDevice->sendBuffer + nextByteIndex),
                bytesToTransfer);
        nextByteIndex += bytesToTransfer;
    }
}

void initializeUsb(CanUsbDevice* usbDevice) {
    Serial.print("Initializing USB...  ");
    usbDevice->device.InitializeSystem(true);
    while(usbDevice->device.GetDeviceState() < CONFIGURED_STATE);
    Serial.println("Done.");
}

USB_HANDLE armForRead(CanUsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    return usbDevice->device.GenRead(usbDevice->endpoint, (uint8_t*)buffer,
            usbDevice->endpointSize);
}

USB_HANDLE readFromHost(CanUsbDevice* usbDevice, USB_HANDLE handle,
        void (*callback)(char*)) {
    if(!usbDevice->device.HandleBusy(handle)) {
        // TODO see #569
        delay(50);
        callback(usbDevice->receiveBuffer);
        return armForRead(usbDevice, usbDevice->receiveBuffer);
    }
    return handle;
}
