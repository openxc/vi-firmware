#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <string.h>
#include "chipKITUSBDevice.h"

#define USB_PACKET_SIZE 64
#define MESSAGE_FORMAT "{\"name\":\"%s\",\"value\":%f}\r\n"
// TODO how to enforce this max? or just make it large enough...
#define MESSAGE_VALUE_MAX_LENGTH 6

#define DATA_ENDPOINT 1
#define DATA_ENDPOINT_BUFFER_SIZE 65

const int MESSAGE_FORMAT_LENGTH = strlen(MESSAGE_FORMAT);

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;

void initializeUsb();
void sendMessage(uint8_t* message, int message_size);
static boolean usbCallback(USB_EVENT event, void *pdata, word size);

extern USBDevice USB_DEVICE;
extern USB_HANDLE USB_INPUT_HANDLE;

#endif // _USBUTIL_H_
