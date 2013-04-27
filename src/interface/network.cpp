#include "interface/network.h"
#include "util/log.h"
#include <stddef.h>

void openxc::interface::network::initializeNetworkCommon(NetworkDevice* device) {
    if(device != NULL) {
        debug("Initializing Network...");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
    }
}
