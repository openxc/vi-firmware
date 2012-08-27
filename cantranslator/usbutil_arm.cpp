#include "usbutil.h"
#include "LPC17xx.h"
#include "usbapi.h"
#include <algorithm>
#include <stdio.h>

static U8 abBulkBuf[64];

void sendMessage(CanUsbDevice* usbDevice, uint8_t* message, int messageSize) {
#ifdef DEBUG
    printf("sending message: %s", message);
#endif

    strncpy(usbDevice->sendBuffer, (char*)message, messageSize);
    usbDevice->sendBuffer[messageSize] = '\n';
    messageSize += 1;

#ifdef BLUETOOTH
    // Serial transfers are really, really slow, so we don't want to send unless
    // explicitly set to do so at compile-time. Alternatively, we could send
    // on serial whenever we detect below that we're probably not connected to a
    // USB device, but explicit is better than implicit.
    usbDevice->serial->device->write((const uint8_t*)usbDevice->sendBuffer, messageSize);
#else
    int nextByteIndex = 0;
    while(nextByteIndex < messageSize) {
        int bytesToTransfer = std::min(MAX_USB_PACKET_SIZE, messageSize - nextByteIndex);
        U8 currentByte = *(U8*)(usbDevice->sendBuffer + nextByteIndex);
        fifo_put(&USB_DEVICE.transmitQueue, currentByte);
        nextByteIndex += bytesToTransfer;
    }
#endif
}

static void handleBulkIn(U8 endpoint, U8 endpointStatus) {
	int i, length;

	if (fifo_avail(&USB_DEVICE.transmitQueue) == 0) {
		// no more data, disable further NAK interrupts until next USB frame
		USBHwNakIntEnable(0);
		return;
	}

	// get bytes from transmit FIFO into intermediate buffer
	for (i = 0; i < MAX_USB_PACKET_SIZE; i++) {
		if (!fifo_get(&USB_DEVICE.transmitQueue, (U8*)&USB_DEVICE.sendBuffer[i])) {
			break;
		}
	}
	length = i;

	if(length > 0) {
		USBHwEPWrite(endpoint, (U8*)USB_DEVICE.sendBuffer, length);
	}
}

void initializeUsb(CanUsbDevice* usbDevice) {
    printf("Initializing USB.....");

	USBInit();
	USBRegisterDescriptors(USB_DESCRIPTORS);
	USBHwRegisterEPIntHandler(_EP01_IN, handleBulkIn);
	// USBHwRegisterEPIntHandler(_EP01_OUT, handleBulkOut);
	// TODO how to do override the built-in empty loop IRQ handler?
    // NVIC_EnableIRQ(USB_IRQn);

	USBHwConnect(TRUE);

    printf("Done.");
}

void* armForRead(CanUsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    // return usbDevice->device.GenRead(usbDevice->endpoint, (uint8_t*)buffer,
            // usbDevice->endpointSize);
    return NULL;
}

void* readFromHost(CanUsbDevice* usbDevice, void* handle,
        bool (*callback)(char*)) {
    // if(!usbDevice->device.HandleBusy(handle)) {
        // // TODO see #569
        // delay(500);
        // if(usbDevice->receiveBuffer[0] != NULL) {
            // strncpy((char*)(usbDevice->packetBuffer +
                        // usbDevice->packetBufferIndex), usbDevice->receiveBuffer,
                        // MAX_USB_PACKET_SIZE);
            // usbDevice->packetBufferIndex += MAX_USB_PACKET_SIZE;
            // processBuffer(usbDevice->packetBuffer,
                // &usbDevice->packetBufferIndex, PACKET_BUFFER_SIZE, callback);
        // }
        // return armForRead(usbDevice, usbDevice->receiveBuffer);
    // }
    // return handle;
    return NULL;
}
