#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "emqueue.h"
#include "commands.h"

QUEUE_DECLARE(uint8_t, 256)

namespace openxc {
namespace util {
namespace bytebuffer {

/* Public: Pass the buffer in the queue to the callback, which should return
 * true if an OpenXC message is found and processed, then reset the queue back
 * to empty. If no message is found, keep the queue intact unless the
 * queue is full or corrupted (i.e. it has a NULL character but we stil didn't
 * find an OpenXC message), reset it back to empty.
 *
 * queue - The queue of bytes to check for a message.
 * callback - A function that will return true if an OpenXC message is found in
 *          the queue.
 */
void processQueue(QUEUE_TYPE(uint8_t)* queue,
                openxc::commands::IncomingMessageCallback callback);

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
