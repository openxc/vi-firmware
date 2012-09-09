#include "buffers.h"
#include "strutil.h"
#include "log.h"

void processQueue(ByteQueue* queue, bool (*callback)(uint8_t*)) {
    uint8_t snapshot[queue_length(queue)];
    queue_snapshot(queue, snapshot);
    if(callback == NULL) {
        debug("Callback is NULL (%p) -- unable to handle queue at %p\r\n",
                callback, queue);
        return;
    }

    if(callback(snapshot)) {
        queue_init(queue);
    } else if(queue_full(queue)) {
        debug("Incoming write is too long\r\n");
        queue_init(queue);
    } else if(strnchr((char*)snapshot, sizeof(snapshot) - 1, '\0') != NULL) {
        debug("Incoming buffered write corrupted (%s) -- clearing buffer\r\n",
                snapshot);
        queue_init(queue);
    }
}
