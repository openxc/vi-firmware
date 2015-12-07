#include "ringbuffer.h"

#warning "reader and writer may only write the tail and head, respectively, but they do read the other's pointer....no guarantee this is atomic!...are locks the only solution or is the sequence LW-LW-SW ok?"

void RingBuffer_Initialize(RingBuffer_t* ring, char* data, uint32_t size) {
    ring->head = 0;
    ring->tail = 0;
    ring->size = size;
    ring->start = data;
    ring->init = true;
}

uint32_t RingBuffer_Write(RingBuffer_t* ring, char* data, uint32_t size) {

    uint32_t i; // write counter

    // make sure ring buffer is initialized
    if(!ring->init)
        return 0;

    // ensure we have free space to add all the bytes
    if(RingBuffer_FreeSpace(ring) < size)
        return 0;

    // write the bytes
    for(i = 0; i < size; ++i)
    {
        *(ring->start+ring->head) = data[i];
        ring->head = (uint32_t)(ring->head + 1) % ring->size;
    }

    return i;

}

uint32_t RingBuffer_Read(RingBuffer_t* ring, char* data, uint32_t size) {

    uint32_t i; // read counter
    uint32_t s; // stored size
    uint32_t r; // read size

    // make sure ring buffer is initialized
    if(!ring->init)
        return false;

    // read as many bytes as we can
    s = RingBuffer_UsedSpace(ring);
    r = s < size ? s : size;
    for(i = 0; i < r; ++i)
    {
        *data++ = *(ring->start+ring->tail);
        ring->tail = (uint32_t)(ring->tail + 1) % ring->size;
    }

    return i;

}

uint32_t RingBuffer_Peek(RingBuffer_t* ring, char* data, uint32_t size) {

    uint32_t i; // read counter
    uint32_t s; // stored size
    uint32_t r; // read size
    uint32_t tail;  // peek tail (so we don't modify the ring tail)

    // make sure ring buffer is initialized
    if(!ring->init)
        return false;

    // read as many bytes as we can
    s = RingBuffer_UsedSpace(ring);
    r = s < size ? s : size;
    tail = ring->tail;
    for(i = 0; i < r; ++i)
    {
        *data++ = *(ring->start+tail);
        tail = (uint32_t)(tail + 1) % ring->size;
    }

    return i;
    
}

uint32_t RingBuffer_FreeSpace(RingBuffer_t* ring) {
    return (uint32_t)(ring->tail - ring->head - 1)&(ring->size - 1);
}

uint32_t RingBuffer_UsedSpace(RingBuffer_t* ring) {
    return ring->size - RingBuffer_FreeSpace(ring) - 1; // -1 slot always open
}

void RingBuffer_Clear(RingBuffer_t* ring) {
    ring->head = 0;
    ring->tail = 0;
}