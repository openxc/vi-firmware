#include "buffers.h"
#include "strutil.h"
#include "WProgram.h"

void processQueue(ByteQueue* queue, bool (*callback)(uint8_t*)) {
    uint8_t snapshot[MAX_QUEUE_LENGTH];
    queue_snapshot(queue, snapshot);
    if(callback(snapshot)) {
        queue_init(queue);
    } else if(queue_full(queue)) {
        Serial.println("Incoming write is too long");
        queue_init(queue);
    } else if(strnchr((char*)snapshot, queue_length(queue), NULL) != NULL) {
        Serial.println("Incoming buffered write corrupted -- clearing buffer");
        queue_init(queue);
    }
}
