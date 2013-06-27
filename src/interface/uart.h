#ifndef _UARTUTIL_H_
#define _UARTUTIL_H_

#include "util/bytebuffer.h"

namespace openxc {
namespace interface {
namespace uart {

extern const int MAX_MESSAGE_SIZE;
extern const int BAUD_RATE;

/* Public: A container for a UART connection with queues for both input and
 * output.
 *
 * sendQueue - A queue of bytes that need to be sent out over UART.
 * receiveQueue - A queue of bytes that have been received via UART but not yet
 *      processed.
 * device - A pointer to the hardware UART device to use for OpenXC messages.
 */
typedef struct {
    int baudRate;
    // device to host
    QUEUE_TYPE(uint8_t) sendQueue;
    // host to device
    QUEUE_TYPE(uint8_t) receiveQueue;
    void* controller;
} UartDevice;

/* Public: Try to read a message from the UART device (or grab data that's
 * already been received and queued in the receiveQueue) and process it using
 * the given callback.
 *
 * device - The UART device to check for incoming data.
 * callback - A function to call with any received data.
 */
void read(UartDevice* device, bool (*callback)(uint8_t*));

/* Public: Perform platform-agnostic UART initialization.
 */
void initializeCommon(UartDevice* device);

/* Public: Initializes the UART device at at 115200 baud rate and initializes
 * the receive buffer.
 */
void initialize(UartDevice* device);

/* Public: Common procedures to run after initializing UART.
 */
void postInitializeCommon(UartDevice* device);

/* Public: Send any bytes in the outgoing data queue out over the UART
 * connection.
 *
 * This function may or may not be blocking - it's implementation dependent.
 */
void processSendQueue(UartDevice* device);

/* Public: Check the connection status of a UART receiver.
 *
 * Returns true if UART is connected.
 */
bool connected(UartDevice* device);

} // namespace uart
} // namespace interface
} // namespace openxc

#endif // _UARTUTIL_H_
