#ifdef LPC1768

#include "usbutil.h"
#include "log.h"
#include <algorithm>
#include <stdio.h>

extern "C" {
#include "LPC17xx.h"
#include "usbapi.h"
}

// TODO how can we both use USB interrupts and not rely on this global?
extern UsbDevice USB_DEVICE;

void processInputQueue(UsbDevice* usbDevice) {
}

static void handleBulkIn(U8 endpoint, U8 endpointStatus) {
    if(queue_empty(&USB_DEVICE.sendQueue)) {
        // no more data, disable further NAK interrupts until next USB frame
        USBHwNakIntEnable(0);
        return;
    }

    // get bytes from transmit FIFO into intermediate buffer
    int byteCount = 0;
    while(!queue_empty(&USB_DEVICE.sendQueue) && byteCount < 64) {
        USB_DEVICE.sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &USB_DEVICE.sendQueue);
    }

    if(byteCount > 0) {
        USBHwEPWrite(endpoint, (U8*)USB_DEVICE.sendBuffer, byteCount);
    }
}

static void USBFrameHandler(U16 wFrame)                                          {
    if (!queue_empty(&USB_DEVICE.sendQueue)) {
        // data available, enable NAK interrupt on bulk in
        USBHwNakIntEnable(INACK_BI);
        handleBulkIn(_EP01_IN, 0);
    }
}

void initializeUsb(UsbDevice* usbDevice) {
    debug("Initializing USB.....");

    USBInit();
    USBRegisterDescriptors(USB_DESCRIPTORS);
    USBHwRegisterEPIntHandler(_EP01_IN, handleBulkIn);
    USBHwNakIntEnable(INACK_BI);
    USBHwRegisterFrameHandler(USBFrameHandler);
    // USBHwRegisterEPIntHandler(_EP01_OUT, handleBulkOut);
    // TODO how to do override the built-in empty loop IRQ handler?
    // NVIC_EnableIRQ(USB_IRQn);

    USBHwConnect(TRUE);

    debug("Done.");
}

void armForRead(UsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    // return usbDevice->device.GenRead(usbDevice->endpoint, (uint8_t*)buffer,
            // usbDevice->endpointSize);
}

void readFromHost(UsbDevice* usbDevice, void* handle,
        bool (*callback)(char*)) {
    // if(!usbDevice->device.HandleBusy(handle)) {
        // // TODO see #569
        // delay(500);
        // if(usbDevice->receiveBuffer[0] != NULL) {
            // strncpy((char*)(usbDevice->packetBuffer +
                        // usbDevice->packetBufferIndex), usbDevice->receiveBuffer,
                        // MAX_USB_PACKET_SIZE_BYTES);
            // usbDevice->packetBufferIndex += MAX_USB_PACKET_SIZE_BYTES;
            // processBuffer(usbDevice->packetBuffer,
                // &usbDevice->packetBufferIndex, PACKET_BUFFER_SIZE, callback);
        // }
        // return armForRead(usbDevice, usbDevice->receiveBuffer);
    // }
    // return handle;
}

#endif // LPC1768
