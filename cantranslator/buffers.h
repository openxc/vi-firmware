#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "queue.h"

/* Public: Pass the buffer in the queue to the callback, which should return
 * true if an OpenXC message is found and processed, then reset the queue back
 * to empty. If no message is found, keep the queue intact unless the
 * queue is full or corrupted (i.e. it has a NULL character but we stil didn't
 * find an OpenXC message), reset it back to empty.
 *
 * queue - The queue of bytes to check for a message.
 * callback - A function that will return true if an OpenXC message is found in
 * 			the queue.
 */
void processQueue(ByteQueue* queue, bool (*callback)(uint8_t*));

#endif // _BUFFERS_H_
