/* 
 * File:   ringbuffer.h
 * Author: Michael
 *
 * Created on May 4, 2015, 2:28 PM
 */

#ifndef RINGBUFFER_H
#define    RINGBUFFER_H

#ifdef    __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// make sure this data structure is word-aligned...we want atomic accesses
    // for example buffering between application and ISR
// when porting to another platform, make sure accesses are atomic!
typedef struct {
    uint32_t init;
    uint32_t head;
    uint32_t tail;
    char* start;
    uint32_t size;
}RingBuffer_t;

void RingBuffer_Initialize(RingBuffer_t* ring, char* data, uint32_t size);
uint32_t RingBuffer_Write(RingBuffer_t* ring, char* data, uint32_t size);
uint32_t RingBuffer_Read(RingBuffer_t* ring, char* data, uint32_t size);
uint32_t RingBuffer_FreeSpace(RingBuffer_t* ring);
uint32_t RingBuffer_UsedSpace(RingBuffer_t* ring);
uint32_t RingBuffer_Peek(RingBuffer_t* ring, char* data, uint32_t size);
void RingBuffer_Clear(RingBuffer_t* ring);

#ifdef    __cplusplus
}
#endif

#endif    /* RINGBUFFER_H */

