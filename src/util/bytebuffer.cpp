#include "bytebuffer.h"
#include "strutil.h"
#include "util/log.h"

QUEUE_DEFINE(uint8_t)

using openxc::util::log::debug;
using openxc::util::bytebuffer::IncomingMessageCallback;

bool openxc::util::bytebuffer::processQueue(QUEUE_TYPE(uint8_t)* queue,
        IncomingMessageCallback callback) {
    int length = QUEUE_LENGTH(uint8_t, queue);
    if(length == 0) {
        return false;
    }

    uint8_t snapshot[length];
    QUEUE_SNAPSHOT(uint8_t, queue, snapshot, length);
    if(callback == NULL) {
        debug("Callback is NULL (%p) -- unable to handle queue at %p",
                callback, queue);
        return false;
    }

    size_t parsedLength = callback(snapshot, length);
    for(size_t i = 0; i < parsedLength; i++) {
        QUEUE_POP(uint8_t, queue);
    }

    if(QUEUE_FULL(uint8_t, queue)) {
        debug("Incoming write is too long - dumping queue");
        QUEUE_INIT(uint8_t, queue);
    }
    return parsedLength > 0;
}

bool openxc::util::bytebuffer::messageFits(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    return queue != NULL && QUEUE_AVAILABLE(uint8_t, queue) >= messageSize + 2;
}

bool openxc::util::bytebuffer::conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(messageFits(queue, message, messageSize)) {
        for(int i = 0; i < messageSize; i++) {
            QUEUE_PUSH(uint8_t, queue, (uint8_t)message[i]);
        }
        return true;
    }
    return false;
}
