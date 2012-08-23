#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_QUEUE_LENGTH  128

// Internal pointers must have 1 more so we can tell the difference between full
// and empty
#define MAX_INTERNAL_QUEUE_LENGTH  (MAX_QUEUE_LENGTH + 1)

typedef struct {
    int     head;
    int     tail;
    uint8_t elements[MAX_INTERNAL_QUEUE_LENGTH];
} ByteQueue;

void queue_init(ByteQueue* queue);
bool queue_push(ByteQueue* queue, uint8_t value);
uint8_t queue_pop(ByteQueue* queue);
bool  queue_full(ByteQueue* queue);
bool  queue_empty(ByteQueue* queue);
int queue_length(ByteQueue* queue);

#endif // _QUEUE_H_
