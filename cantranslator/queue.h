#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_QUEUE_LENGTH  128

typedef struct {
    int     head;
    int     tail;
    uint8_t elements[MAX_QUEUE_LENGTH];
} ByteQueue;

void queue_init(ByteQueue* queue);
bool queue_push(ByteQueue* queue, uint8_t value);
uint8_t queue_pop(ByteQueue* queue);
bool  queue_full(ByteQueue* queue);
bool  queue_empty(ByteQueue* queue);
int queue_length(ByteQueue* queue);

#endif // _QUEUE_H_
