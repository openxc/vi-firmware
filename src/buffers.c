#include "buffers.h"
#include "strutil.h"
#include "log.h"

QUEUE_DEFINE(uint8_t)

void processQueue(ByteQueue* queue, bool (*callback)(uint8_t*)) {
    int length = QUEUE_LENGTH(uint8_t, queue);
    if(length == 0) {
        return;
    }

    uint8_t snapshot[length];
    QUEUE_SNAPSHOT(uint8_t, queue, snapshot);
    if(callback == NULL) {
        debug("Callback is NULL (%p) -- unable to handle queue at %p\r\n",
                callback, queue);
        return;
    }

    if(callback(snapshot)) {
        QUEUE_INIT(uint8_t, queue);
    } else if(QUEUE_FULL(uint8_t, queue)) {
        debug("Incoming write is too long\r\n");
        QUEUE_INIT(uint8_t, queue);
    } else if(strnchr((char*)snapshot, sizeof(snapshot) - 1, '\0') != NULL) {
        debug("Incoming buffered write corrupted (%s) -- clearing buffer\r\n",
                snapshot);
        QUEUE_INIT(uint8_t, queue);
    }
}

bool conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(QUEUE_AVAILABLE(uint8_t, queue) < messageSize + 2) {
        return false;
    }

    for(int i = 0; i < messageSize; i++) {
        QUEUE_PUSH(uint8_t, queue, (uint8_t)message[i]);
    }
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\r');
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\n');
    return true;
}
