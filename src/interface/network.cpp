#include "interface/network.h"
#include "util/log.h"
#include <stddef.h>

void openxc::interface::network::initializeCommon(NetworkDevice* device) {
    if(device != NULL) {
        debug("Initializing Network...");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
        device->allowRawWrites =
#ifdef NETWORK_ALLOW_RAW_WRITE
            true
#else
            false
#endif
            ;
    }
}
