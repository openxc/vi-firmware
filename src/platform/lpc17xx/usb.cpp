#include "interface/usb.h"
#include <stdio.h>
#include "util/log.h"
#include "util/bytebuffer.h"
#include "gpio.h"
#include "usb_config.h"

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"

extern "C" {
#include "bsp.h"
}

#define VBUS_PORT 1
#define VBUS_PIN 30
#define VBUS_FUNCNUM 2

#define USB_DM_PORT 0
#define USB_DM_PIN 30
#define USB_DM_FUNCNUM 1

#define USB_HOST_DETECT_DEBOUNCE_VALUE 10000

#define USB_CONNECT_PORT 2
#define USB_CONNECT_PIN 9

namespace gpio = openxc::gpio;

using openxc::interface::usb::UsbDevice;
using openxc::util::bytebuffer::processQueue;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;

extern UsbDevice USB_DEVICE;
extern bool handleControlRequest(uint8_t);

void configureEndpoints() {
	#ifndef DATA_LOGGER
		Endpoint_ConfigureEndpoint(OUT_ENDPOINT_NUMBER, EP_TYPE_BULK,
				ENDPOINT_DIR_OUT, DATA_ENDPOINT_SIZE, ENDPOINT_BANK_DOUBLE);
		Endpoint_ConfigureEndpoint(IN_ENDPOINT_NUMBER, EP_TYPE_BULK,
				ENDPOINT_DIR_IN, DATA_ENDPOINT_SIZE, ENDPOINT_BANK_DOUBLE);
	#endif

}

extern "C" {

void EVENT_USB_Device_Disconnect() {
    debug("USB no longer detected - marking unconfigured");
    USB_DEVICE.configured = false;
}

void EVENT_USB_Device_ControlRequest() {
	#ifndef DATA_LOGGER
		if(!(Endpoint_IsSETUPReceived())) {
			return;
		}

		handleControlRequest(USB_ControlRequest.bRequest);
	#endif
   
}

void EVENT_USB_Device_ConfigurationChanged(void) {
    configureEndpoints();
    // don't use debug logging in here because if the function doesn't return
    // quick enough, the control transfer can fail because it times out
    USB_DEVICE.configured = true;
}

}

/* Private: Flush any queued data out to the USB host. */
static void sendToHost(UsbDevice* usbDevice) {
   #ifndef DATA_LOGGER
		if(!usbDevice->configured) {
			return;
		}

		uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
		Endpoint_SelectEndpoint(IN_ENDPOINT_NUMBER);
		if(!Endpoint_IsINReady() || QUEUE_EMPTY(uint8_t, &usbDevice->sendQueue)) {
			return;
		}

		// get bytes from transmit FIFO into intermediate buffer
		int byteCount = 0;
		while(!QUEUE_EMPTY(uint8_t, &usbDevice->sendQueue)
				&& byteCount < USB_SEND_BUFFER_SIZE) {
			usbDevice->sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &usbDevice->sendQueue);
		}

		if(byteCount > 0) {
			Endpoint_Write_Stream_LE(usbDevice->sendBuffer, byteCount, NULL);
		}
		Endpoint_ClearIN();
		Endpoint_SelectEndpoint(previousEndpoint);
   #endif
}

/* Private: Detect if USB VBUS is active.
 *
 * This isn't useful if there's no diode between an external 12v/9v power supply
 * (e.g. vehicle power from OBD-II) and the 5v rail, because then VBUS high when
 * the power is powered on regardless of the status of USB. In that situation,
 * you can fall back to the usbHostDetected() function instead.
 *
 * Returns true if VBUS is high.
 */
bool vbusDetected() {
    return gpio::getValue(VBUS_PORT, VBUS_PIN) != GPIO_VALUE_LOW;
}

/* Private: Detect if a USB host is actually attached, regardless of VBUS.
 *
 * This is a bit hacky, as normally you should rely on VBUS to detect if USB is
 * connected. See vbusDetected() for reasons why we need this workaround on the
 * current prototype.
 *
 * Returns true of there is measureable activity on the D- USB line.
 */
bool usbHostDetected() {
    static int debounce = 0;

    if(gpio::getValue(USB_DM_PORT, USB_DM_PIN) == GPIO_VALUE_LOW) {
        ++debounce;
    } else {
        debounce = 0;
    }

    if(debounce > USB_HOST_DETECT_DEBOUNCE_VALUE) {
        debounce = 0;
        return false;
    }
    return true;
}

/* Private: Configure I/O pins used to detect if USB is connected to a host. */
void configureUsbDetection() {
    PINSEL_CFG_Type vbusPinConfig;
    vbusPinConfig.Funcnum = VBUS_FUNCNUM;
    vbusPinConfig.Portnum = VBUS_PORT;
    vbusPinConfig.Pinnum = VBUS_PIN;
    vbusPinConfig.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&vbusPinConfig);

    PINSEL_CFG_Type hostDetectPinConfig;
    hostDetectPinConfig.Funcnum = USB_DM_FUNCNUM;
    hostDetectPinConfig.Portnum = USB_DM_PORT;
    hostDetectPinConfig.Pinnum = USB_DM_PIN;
    hostDetectPinConfig.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&hostDetectPinConfig);
}

void openxc::interface::usb::sendControlMessage(uint8_t* data, uint8_t length) {
   #ifndef DATA_LOGGER
		uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
		Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE(data, length);
		Endpoint_ClearOUT();

		Endpoint_SelectEndpoint(previousEndpoint);
	#endif
}


void openxc::interface::usb::processSendQueue(UsbDevice* usbDevice) {
    #ifndef DATA_LOGGER
		USB_USBTask();

		if(usbDevice->configured && (USB_DeviceState != DEVICE_STATE_Configured
					|| !vbusDetected() || !usbHostDetected())) {
			EVENT_USB_Device_Disconnect();
		} else {
			sendToHost(usbDevice);
		}
	#endif
}

void openxc::interface::usb::initialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
    USB_Init();
    ::USB_Connect();
    configureUsbDetection();

    debug("Done.");
}

void openxc::interface::usb::read(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) {
	#ifndef DATA_LOGGER
		uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
		Endpoint_SelectEndpoint(OUT_ENDPOINT_NUMBER);

		while(Endpoint_IsOUTReceived()) {
			while(Endpoint_BytesInEndpoint()) {
				if(!QUEUE_PUSH(uint8_t, &usbDevice->receiveQueue,
							Endpoint_Read_8())) {
					debug("Dropped write from host -- queue is full");
				}
			}
			processQueue(&usbDevice->receiveQueue, callback);
			Endpoint_ClearOUT();
		}
		Endpoint_SelectEndpoint(previousEndpoint);
	#endif
}

void openxc::interface::usb::deinitialize(UsbDevice* usbDevice) {
    // Turn off USB connection status LED
    gpio::setValue(USB_CONNECT_PORT, USB_CONNECT_PIN, GPIO_VALUE_HIGH);
}
