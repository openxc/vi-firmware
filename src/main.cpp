#include "serialutil.h"
#include "usbutil.h"
#include "listener.h"

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_ENDPOINT 1

void setup();
void loop();

const char* VERSION = "2.0-pre";

#ifdef __CHIPKIT__
SerialDevice serialDevice = {&Serial1};
#else
SerialDevice serialDevice;
#endif // __CHIPKIT__

UsbDevice USB_DEVICE = {
#ifdef CHIPKIT
    USBDevice(usbCallback),
#endif // CHIPKIT
    DATA_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Listener listener = {&USB_DEVICE, &serialDevice};

int main(void) {
#ifdef CHIPKIT
	init();
#endif
	setup();

	for (;;)
		loop();

	return 0;
}
