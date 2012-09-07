#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "usbutil.h"
#include "serialutil.h"

/*
 * usb -
 * serial -
 */
struct Listener {
    UsbDevice* usb;
    SerialDevice* serial;
};

void sendMessage(Listener* listener, uint8_t* message, int messageSize);

void processListenerQueues(Listener* listener);

#endif // _LISTENER_H_
