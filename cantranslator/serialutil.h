#ifndef _SERIALUTIL_H_
#define _SERIALUTIL_H_

#include "WProgram.h"

#define SERIAL_BUFFER_SIZE 64 * 4

/* Public: a container for a CAN translator Serial device and associated
 * metadata.
 *
 * device - A pointer to the hardware serial device to use for OpenXC messages.
 */
struct SerialDevice {
    HardwareSerial* device;
    // host to device
    char receiveBuffer[SERIAL_BUFFER_SIZE];
    int receiveBufferIndex;
};

/* Public: Try to read a message from the serial device and process it using the
 * callback.
 *
 * Read as many bytes as are available on the serial device and add them to a
 * running buffer.
 *
 * serial - The serial device to read from.
 * callback - a function that handles incoming messages.
 */
void readFromSerial(SerialDevice* serial, bool (*callback)(char*));

/* Public: Initializes the serial device at at 115200 baud rate and initializes
 * the receive buffer.
 */
void initializeSerial(SerialDevice* serial);

#endif // _SERIALUTIL_H_
