#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define QUEUE_MAX_LENGTH(type) queue_##type##_max_length
#define QUEUE_MAX_INTERNAL_LENGTH(type) queue_##type##_max_internal_length

#define QUEUE_DECLARE(type, max_length) \
static const int queue_##type##_max_length = max_length; \
static const int queue_##type##_max_internal_length = max_length + 1; \
typedef struct queue_##type##_s { \
    int head; \
    int tail; \
    type elements[max_length + 1]; \
} queue_##type; \
\
bool queue_##type##_push(queue_##type* queue, type value); \
\
type queue_##type##_pop(queue_##type* queue); \
\
type queue_##type##_peek(queue_##type* queue); \
void queue_##type##_init(queue_##type* queue); \
int queue_##type##_length(queue_##type* queue); \
int queue_##type##_available(queue_##type* queue); \
bool queue_##type##_full(queue_##type* queue); \
bool queue_##type##_empty(queue_##type* queue); \
void queue_##type##_snapshot(queue_##type* queue, type* snapshot);

#define QUEUE_DEFINE(type) \
bool queue_##type##_push(queue_##type* queue, type value) { \
    int next = (queue->head + 1) % (queue_##type##_max_internal_length); \
    if (next == queue->tail) { \
        return false; \
    } \
    \
    queue->elements[queue->head] = value; \
    queue->head = next; \
    \
    return true; \
} \
\
type queue_##type##_pop(queue_##type* queue) { \
    int next = (queue->tail + 1) % queue_##type##_max_internal_length; \
    type value = queue->elements[queue->tail]; \
    queue->tail = next; \
\
    return value; \
} \
\
type queue_##type##_peek(queue_##type* queue) { \
    return queue->elements[queue->tail]; \
} \
\
void queue_##type##_init(queue_##type* queue) { \
    queue->head = queue->tail = 0; \
} \
\
int queue_##type##_length(queue_##type* queue) { \
    return (queue_##type##_max_internal_length + queue->head - queue->tail) \
            % queue_##type##_max_internal_length; \
} \
\
int queue_##type##_available(queue_##type* queue) { \
    return queue_##type##_max_length - queue_##type##_length(queue); \
} \
\
bool queue_##type##_full(queue_##type* queue) { \
    return queue_##type##_length(queue) == queue_##type##_max_length; \
} \
\
bool queue_##type##_empty(queue_##type* queue) { \
    return queue_##type##_length(queue) == 0; \
} \
\
void queue_##type##_snapshot(queue_##type* queue, type* snapshot) { \
    int i; \
    for(i = 0; i < queue_##type##_length(queue); i++) { \
        snapshot[i] = queue->elements[ \
            (queue->tail + i) % queue_##type##_max_internal_length]; \
    } \
}

#define QUEUE_TYPE(type) queue_##type
#define QUEUE_PUSH(type, queue, value) queue_##type##_push(queue, value)
#define QUEUE_POP(type, queue) queue_##type##_pop(queue)
#define QUEUE_PEEK(type, queue) queue_##type##_peek(queue)
#define QUEUE_INIT(type, queue) queue_##type##_init(queue)
#define QUEUE_LENGTH(type, queue) queue_##type##_length(queue)
#define QUEUE_AVAILABLE(type, queue) queue_##type##_available(queue)
#define QUEUE_FULL(type, queue) queue_##type##_full(queue)
#define QUEUE_EMPTY(type, queue) queue_##type##_empty(queue)
#define QUEUE_SNAPSHOT(type, queue, snapshot) queue_##type##_snapshot(queue, \
        snapshot)

#ifdef __cplusplus
}
#endif

#endif // _QUEUE_H_
