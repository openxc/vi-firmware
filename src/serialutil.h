#ifndef _SERIALUTIL_H_
#define _SERIALUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __PIC32__
#include "WProgram.h"
#endif // __PIC32__

#include "queue.h"

#define MAX_MESSAGE_SIZE 128

/* Public: a container for a CAN translator Serial device and associated
 * metadata.
 *
 * device - A pointer to the hardware serial device to use for OpenXC messages.
 */
typedef struct {
#ifdef __PIC32__
    HardwareSerial* device;
#endif // __PIC32__
    // device to host
    ByteQueue sendQueue;
    // host to device
    ByteQueue receiveQueue;
} SerialDevice;

/* Public: Try to read a message from the serial device and process it using the
 * callback.
 *
 * Read as many bytes as are available on the serial device and add them to a
 * running buffer.
 *
 * serial - The serial device to read from.
 * callback - a function that handles incoming messages.
 */
void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*));

/* Public: Initializes the serial device at at 115200 baud rate and initializes
 * the receive buffer.
 */
void initializeSerial(SerialDevice* serial);

void processSerialSendQueue(SerialDevice* device);

#ifdef __cplusplus
}
#endif

#endif // _SERIALUTIL_H_
