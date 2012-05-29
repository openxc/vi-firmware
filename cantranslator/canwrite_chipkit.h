#ifndef _CANWRITE_CHIPKIT_H_
#define _CANWRITE_CHIPKIT_H_

#include "canutil.h"
#include "cJSON.h"

void sendCanSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount);

#endif // _CANWRITE_CHIPKIT_H_
