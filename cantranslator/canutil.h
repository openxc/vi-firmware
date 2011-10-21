#ifndef _CANUTIL_H_
#define _CANUTIL_H_

#include "WProgram.h"
#include "chipKITCAN.h"

void configure_hs_filters(CAN *canMod);

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

#endif // _CANUTIL_U
