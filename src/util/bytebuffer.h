#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "emqueue.h"
#include "commands/commands.h"

QUEUE_DECLARE(uint8_t, 320)

namespace openxc {
namespace util {
namespace bytebuffer {

/* Public: The type signature for a callback to receive new command data.
 *
 * buffer - The received command buffer. It may contain an incomplete command, a
 *      complete command, or more. Parse only one.
 * length - The total length of the buffer.
 *
 * The callback should return the number of bytes read from the buffer for a
 * message, if any was found.
 */
typedef size_t (*IncomingMessageCallback)(uint8_t* buffer, size_t length);

/* Public: Search for a complete message in the queue, remove it and pass it to
 * the callback. If no message is found, reset the queue back to empty if it's
 * full.
 *
 * queue - The queue of bytes to check for a message.
 * callback - A function that will return true if an OpenXC message is found in
 *          the queue.
 *
 * Returns true if a completed message was found in the queue and removed.
 */
bool processQueue(QUEUE_TYPE(uint8_t)* queue, IncomingMessageCallback callback);

/* Public: Add the message to the byte queue if there is room.
 *
 * queue - The queue to add the message.
 * message - The message to attempt to enqueue.
 * messageSize - The length of the message.
 *
 * Returns true if the message was able to fit in the queue and was added.
 * Returns false otherwise, or if queue is NULL.
 */
bool conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize);

/* Public: Check if a message plus a CRLF will fit in the byte queue.
 *
 * queue - The queue to add the message.
 * message - The message to check for a good fit.
 * messageSize - The length of the message.
 *
 * Returns true if the message will able to fit in the queue.
 * Returns false otherwise, or if queue is NULL.
 */
bool messageFits(QUEUE_TYPE(uint8_t)* queue, uint8_t* message, int messageSize);

} // namespace bytebuffer
} // namespace util
} // namespace openxc

#endif // _BUFFERS_H_
