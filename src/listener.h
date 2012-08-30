#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "usbutil.h"
#include "serialutil.h"

/*
 *
 * usb -
 * serial - A serial device to use in parallel to USB. TODO Yes, this is
 *      a struct for USB devices, but this is the place where this reference
 *      makes the most sense right now. Since we're actually using the
 *      hard-coded Serial1 object instead of SoftwareSerial as I had initially
 *      planned, we could actually drop this reference altogether.
 */
struct Listener {
    UsbDevice* usb;
    SerialDevice* serial;
};

void sendMessage(Listener* listener, uint8_t* message, int messageSize);

void processListenerQueues(Listener* listener);

#endif // _LISTENER_H_
