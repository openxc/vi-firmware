#include "buffers.h"
#include "strutil.h"
#include "log.h"

void processQueue(ByteQueue* queue, bool (*callback)(uint8_t*)) {
    uint8_t snapshot[QUEUE_MAX_LENGTH(uint8_t)];
    queue_snapshot(queue, snapshot);
    if(callback(snapshot)) {
        queue_init(queue);
    } else if(queue_full(queue)) {
        debug("Incoming write is too long");
        queue_init(queue);
    } else if(strnchr((char*)snapshot, queue_length(queue), NULL) != NULL) {
        debug("Incoming buffered write corrupted -- clearing buffer");
        queue_init(queue);
    }
}
