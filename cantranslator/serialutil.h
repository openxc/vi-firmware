#ifndef _SERIALUTIL_H_
#define _SERIALUTIL_H_

#include "WProgram.h"

#define SERIAL_BUFFER_SIZE 64

/* Public: a container for a CAN translator Serial device and associated
 * metadata.
 */
struct SerialDevice {
    HardwareSerial device;
    // host to device
    char receiveBuffer[SERIAL_BUFFER_SIZE];
};

void readFromSerial(SerialDevice* serial, void (*callback)(char*));

#endif // _SERIALUTIL_H_
