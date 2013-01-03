#include "ethernetutil.h"
#include "log.h"

void initializeEthernetCommon(EthernetDevice* device) {
    if(device != NULL) {
        debug("Initializing Ethernet...");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
    }
}
