#ifndef _CANUTIL_H_
#define _CANUTIL_H_

#include "WProgram.h"
#include "chipKITCAN.h"
#include "bitfield.h"


struct CanFilterMask {
    int number;
    int value;
};

struct CanFilter {
    int number;
    int value;
    int channel;
    int maskNumber;
};

struct CanSignal {
    int id;
    char* genericName;
    int bitPosition;
    int bitSize;
    bool transform;
    float factor;
    float offset;
};

void configureFilters(CAN *canMod, CanFilterMask*, CanFilter*);
void decodeCanSignal(uint8_t* data, CanSignal* signal);
void sendSignal(CanSignal* signal, float value);

#endif // _CANUTIL_H_
