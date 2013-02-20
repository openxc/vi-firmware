#include "usbutil.h"
#include "buffers.h"
#include "log.h"

#define USB_VBUS_ANALOG_INPUT A0
#define USB_HANDLE_MAX_WAIT_COUNT 25000

extern "C" {
extern bool handleControlRequest(uint8_t);
}

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;
extern UsbDevice USB_DEVICE;

boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    // initial connection up to configure will be handled by the default
    // callback routine.
    USB_DEVICE.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        debug("USB Configured");
        USB_DEVICE.configured = true;
        USB_DEVICE.device.EnableEndpoint(USB_DEVICE.inEndpoint,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USB_DEVICE.device.EnableEndpoint(USB_DEVICE.outEndpoint,
                USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        break;

    case EVENT_EP0_REQUEST:
        handleControlRequest(SetupPkt.bRequest);
        break;

    default:
        break;
    }
}

void sendControlMessage(uint8_t* data, uint8_t length) {
    USB_DEVICE.device.EP0SendRAMPtr(data, length, USB_EP0_INCLUDE_ZERO);
}

bool vbusEnabled() {
    return analogRead(USB_VBUS_ANALOG_INPUT) < 100;
}

bool waitForHandle(UsbDevice* usbDevice) {
    int i = 0;
    while(usbDevice->configured &&
            usbDevice->device.HandleBusy(usbDevice->deviceToHostHandle)) {
        ++i;
        if(i > USB_HANDLE_MAX_WAIT_COUNT) {
            // The reason we want to exit this loop early is that if USB is
            // attached and configured, but the host isn't sending an IN
            // requests, we will block here forever. As it is, it still slows
            // down UART transfers quite a bit, so setting configured = false
            // ASAP is important.

            // This can get really noisy when running but I want to leave it in
            // because it' useful to enable when debugging.
            // debug("USB most likely not connected or at least not requesting "
                    // "IN transfers - bailing out of handle waiting");
            return false;
        }
    }
    return true;
}


void processUsbSendQueue(UsbDevice* usbDevice) {
    if(usbDevice->configured && vbusEnabled()) {
        // if there's nothing attached to the analog input it floats at ~828, so
        // if we're powering the board from micro-USB (and the jumper is going
        // to 5v and not the analog input), this is still OK.
        debug("USB no longer detected - marking unconfigured");
        usbDevice->configured = false;
    }

    // Don't touch usbDevice->sendBuffer if there's still a pending transfer
    if(!waitForHandle(usbDevice)) {
        return;
    }

    while(usbDevice->configured &&
            !QUEUE_EMPTY(uint8_t, &usbDevice->sendQueue)) {
        int byteCount = 0;
        while(!QUEUE_EMPTY(uint8_t, &usbDevice->sendQueue) &&
                byteCount < USB_SEND_BUFFER_SIZE) {
            usbDevice->sendBuffer[byteCount++] = QUEUE_POP(uint8_t,
                    &usbDevice->sendQueue);
        }

        int nextByteIndex = 0;
        while(nextByteIndex < byteCount) {
            // Make sure the USB write is 100% complete before messing with this
            // buffer after we copy the message into it - the Microchip library
            // doesn't copy the data to its own internal buffer. See #171 for
            // background on this issue.
            // TODO instead of dropping, replace POP above with a SNAPSHOT
            // and POP off only exactly how many bytes were sent after the
            // fact.
            // TODO in order for this not to fail too often I had to increase
            // the USB_HANDLE_MAX_WAIT_COUNT. that may be OK since now we have
            // VBUS detection.
            if(!waitForHandle(usbDevice)) {
                debug("USB not responding in a timely fashion, dropped data");
                return;
            }

            int bytesToTransfer = min(MAX_USB_PACKET_SIZE_BYTES,
                    byteCount - nextByteIndex);
            usbDevice->deviceToHostHandle = usbDevice->device.GenWrite(
                    usbDevice->inEndpoint,
                    &usbDevice->sendBuffer[nextByteIndex], bytesToTransfer);
            nextByteIndex += bytesToTransfer;
        }
    }
}

void initializeUsb(UsbDevice* usbDevice) {
    initializeUsbCommon(usbDevice);
    usbDevice->device = USBDevice(usbCallback);
    usbDevice->device.InitializeSystem(false);
    pinMode(USB_VBUS_ANALOG_INPUT, INPUT);
    debug("Done.");
}


/* Private: Arm the given endpoint for a read from the device to host.
 *
 * This also puts a NUL char in the beginning of the buffer so you don't get
 * confused that it's still a valid message.
 *
 * device - the CAN USB device to arm the endpoint on
 * buffer - the destination buffer for the next OUT transfer.
 */
void armForRead(UsbDevice* usbDevice, char* buffer) {
    buffer[0] = 0;
    usbDevice->hostToDeviceHandle = usbDevice->device.GenRead(
            usbDevice->outEndpoint, (uint8_t*)buffer,
            usbDevice->outEndpointSize);
}

void readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) {
    if(!usbDevice->device.HandleBusy(usbDevice->hostToDeviceHandle)) {
        if(usbDevice->receiveBuffer[0] != NULL) {
            for(int i = 0; i < usbDevice->outEndpointSize; i++) {
                if(!QUEUE_PUSH(uint8_t, &usbDevice->receiveQueue,
                            usbDevice->receiveBuffer[i])) {
                    debug("Dropped write from host -- queue is full");
                }
            }
            processQueue(&usbDevice->receiveQueue, callback);
        }
        armForRead(usbDevice, usbDevice->receiveBuffer);
    }
}
