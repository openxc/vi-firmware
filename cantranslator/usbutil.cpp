#include "usbutil.h"
#include "buffers.h"

// TODO move this up to cantranslator.pde
USB_HANDLE USB_INPUT_HANDLE = 0;

void sendMessage(CanUsbDevice* usbDevice, uint8_t* message, int messageSize) {
    for(int i = 0; i < messageSize; i++) {
        if(!queue_push(&usbDevice->sendQueue, (uint8_t)message[i])) {
            Serial.println("Dropped an incoming CAN message because "
                    "the USB buffer was full");
            return;
        }
    }
    queue_push(&usbDevice->sendQueue, (uint8_t)'\n');
}

void processInputQueue(CanUsbDevice* usbDevice) {
    while(!queue_empty(&usbDevice->sendQueue)) {

        // Make sure the USB write is 100% complete before messing with this buffer
        // after we copy the message into it - the Microchip library doesn't copy
        // the data to its own internal buffer. See #171 for background on this
        // issue.
        int i = 0;
        while(usbDevice->configured &&
                usbDevice->device.HandleBusy(USB_INPUT_HANDLE)) {
            ++i;
            if(i > 50000) {
                // USB most likely not connected or at least not requesting reads,
                // so we bail as to not block the main loop.
                return;
            }
        }

        int byteCount = 0;
        while(!queue_empty(&usbDevice->sendQueue) && byteCount < 64) {
            usbDevice->sendBuffer[byteCount++] = queue_pop(&usbDevice->sendQueue);
        }

#ifdef BLUETOOTH
        // Serial transfers are really, really slow, so we don't want to send unless
        // explicitly set to do so at compile-time. Alternatively, we could send
        // on serial whenever we detect below that we're probably not connected to a
        // USB device, but explicit is better than implicit.
        usbDevice->serial->device->write((const uint8_t*)usbDevice->sendBuffer,
                byteCount);
#else
        int nextByteIndex = 0;
        while(nextByteIndex < byteCount) {
            while(usbDevice->device.HandleBusy(USB_INPUT_HANDLE));
            int bytesToTransfer = min(USB_PACKET_SIZE, byteCount - nextByteIndex);
            uint8_t* currentByte = (uint8_t*)(usbDevice->sendBuffer
                    + nextByteIndex);
            USB_INPUT_HANDLE = usbDevice->device.GenWrite(usbDevice->endpoint,
                    currentByte, bytesToTransfer);
            nextByteIndex += bytesToTransfer;
        }
#endif
    }
}


void initializeUsb(CanUsbDevice* usbDevice) {
    Serial.print("Initializing USB.....");
    usbDevice->device.InitializeSystem(false);
    queue_init(&usbDevice->sendQueue);
    Serial.println("Done.");
}

USB_HANDLE armForRead(CanUsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    return usbDevice->device.GenRead(usbDevice->endpoint, (uint8_t*)buffer,
            usbDevice->endpointSize);
}

USB_HANDLE readFromHost(CanUsbDevice* usbDevice, USB_HANDLE handle,
        bool (*callback)(char*)) {
    if(!usbDevice->device.HandleBusy(handle)) {
        // TODO see #569
        delay(500);
        if(usbDevice->receiveBuffer[0] != NULL) {
            strncpy((char*)(usbDevice->packetBuffer +
                        usbDevice->packetBufferIndex), usbDevice->receiveBuffer,
                        ENDPOINT_SIZE);
            usbDevice->packetBufferIndex += ENDPOINT_SIZE;
            processBuffer(usbDevice->packetBuffer,
                &usbDevice->packetBufferIndex, PACKET_BUFFER_SIZE, callback);
        }
        return armForRead(usbDevice, usbDevice->receiveBuffer);
    }
    return handle;
}
