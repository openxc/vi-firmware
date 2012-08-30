#ifdef __LPC17XX__

#include "usbutil.h"
#include "log.h"
#include <algorithm>
#include <stdio.h>

extern "C" {
#include "bsp.h"
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
}

extern UsbDevice USB_DEVICE;

void processInputQueue(UsbDevice* usbDevice) {
}

static void handleBulkIn() {
    uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
    Endpoint_SelectEndpoint(DATA_ENDPOINT_NUMBER);
    if(!Endpoint_IsINReady() || queue_empty(&USB_DEVICE.sendQueue)) {
        // no more data, disable further NAK interrupts until next USB frame
        // USBHwNakIntEnable(0);
        return;
    }

    // get bytes from transmit FIFO into intermediate buffer
    int byteCount = 0;
    // TODO try removing the 64 byte limit when using stream sending
    while(!queue_empty(&USB_DEVICE.sendQueue) && byteCount < 64) {
        USB_DEVICE.sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &USB_DEVICE.sendQueue);
    }

    if(byteCount > 0) {
        // TODO LE or BE?
        // TODO may need to pad the end with zeros?
        Endpoint_Write_Stream_LE(USB_DEVICE.sendBuffer, byteCount, NULL);
    }
    Endpoint_ClearIN();
    Endpoint_SelectEndpoint(previousEndpoint);
}

void configureEndpoints() {
    Endpoint_ConfigureEndpoint(DATA_ENDPOINT_NUMBER, EP_TYPE_BULK,
            ENDPOINT_DIR_OUT, DATA_ENDPOINT_SIZE, ENDPOINT_BANK_DOUBLE);
    Endpoint_ConfigureEndpoint(DATA_ENDPOINT_NUMBER, EP_TYPE_BULK,
            ENDPOINT_DIR_IN, DATA_ENDPOINT_SIZE, ENDPOINT_BANK_DOUBLE);
}

void EVENT_USB_Device_ConfigurationChanged(void) {
    configureEndpoints();
}

void USBTask() {
    USB_USBTask();
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

    handleBulkIn();
    // handleBulkOut();
}

void initializeUsb(UsbDevice* usbDevice) {
    debug("Initializing USB.....");

    USB_Init();
    USB_Connect();

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

#endif // __LPC17XX__
