#ifndef _SERIALUTIL_H_
#define _SERIALUTIL_H_

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

/* Public: Sends a message on the bulk transfer endpoint to the host.
 *
 * device - the serial device to send this message on.
 * message - a buffer containing the message to send.
 * messageSize - the length of the message.
 */
void sendMessage(SerialDevice* device, uint8_t* message, int messageSize);

void processInputQueue(SerialDevice* device);

#endif // _SERIALUTIL_H_
