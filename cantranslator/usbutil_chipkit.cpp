#ifdef CHIPKIT

#include "usbutil.h"
#include "buffers.h"
#include "log.h"

void processInputQueue(CanUsbDevice* usbDevice) {
    while(!queue_empty(&usbDevice->sendQueue)) {

        // Make sure the USB write is 100% complete before messing with this buffer
        // after we copy the message into it - the Microchip library doesn't copy
        // the data to its own internal buffer. See #171 for background on this
        // issue.
        int i = 0;
        while(usbDevice->configured &&
                usbDevice->device.HandleBusy(usbDevice->deviceToHostHandle)) {
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
            while(usbDevice->device.HandleBusy(usbDevice->deviceToHostHandle));
            int bytesToTransfer = min(USB_PACKET_SIZE, byteCount - nextByteIndex);
            uint8_t* currentByte = (uint8_t*)(usbDevice->sendBuffer
                    + nextByteIndex);
            usbDevice->deviceToHostHandle = usbDevice->device.GenWrite(
                    usbDevice->endpoint, currentByte, bytesToTransfer);
            nextByteIndex += bytesToTransfer;
        }
#endif
    }
}

void initializeUsb(CanUsbDevice* usbDevice) {
    debug("Initializing USB.....");
    usbDevice->device.InitializeSystem(false);
    queue_init(&usbDevice->sendQueue);
    debug("Done.");
}

void armForRead(CanUsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    usbDevice->hostToDeviceHandle = usbDevice->device.GenRead(
            usbDevice->endpoint, (uint8_t*)buffer, usbDevice->endpointSize);
}

void readFromHost(CanUsbDevice* usbDevice, bool (*callback)(uint8_t*)) {
    if(!usbDevice->device.HandleBusy(usbDevice->hostToDeviceHandle)) {
        // TODO see #569
        delay(500);
        if(usbDevice->receiveBuffer[0] != NULL) {
            for(int i = 0; i < ENDPOINT_SIZE; i++) {
                if(!queue_push(&usbDevice->receiveQueue,
                            usbDevice->receiveBuffer[i])) {
                    debug("Dropped write from host -- queue is full");
                }
            }
            processQueue(&usbDevice->receiveQueue, callback);
        }
        armForRead(usbDevice, usbDevice->receiveBuffer);
    }
}
#endif // CHIPKIT
