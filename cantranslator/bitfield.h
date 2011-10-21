#ifndef _BITFIELD_H_
#define _BITFIELD_H_

#include "WProgram.h"
#include "chipKITCAN.h"

unsigned long get_bit_field(CAN::RxMessageBuffer* message, int startPos,
        int numBits);

#endif // _BITFIELD_H_
