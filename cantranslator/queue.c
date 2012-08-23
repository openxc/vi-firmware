#include "queue.h"

void queue_init(ByteQueue* queue) {
    queue->head = queue->tail = 0;
}

bool queue_push(ByteQueue* queue, uint8_t value) {
	int next = (queue->head + 1) % (MAX_QUEUE_LENGTH + 1);
	if (next == queue->tail) {
		return false;
	}

	queue->elements[queue->head] = value;
	queue->head = next;

	return true;
}

uint8_t queue_pop(ByteQueue* queue) {
	if (queue->head == queue->tail) {
        // TODO error condition?
		return 0;
	}

	int next = (queue->tail + 1) % (MAX_QUEUE_LENGTH + 1);
	uint8_t value = queue->elements[queue->tail];
	queue->tail = next;

    return value;
}


int queue_length(ByteQueue* queue) {
	return queue->head - queue->tail;
}

bool queue_full(ByteQueue* queue) {
    return queue_length(queue) == MAX_QUEUE_LENGTH;
}

bool queue_empty(ByteQueue* queue) {
    return queue_length(queue) == 0;
}
