#include "ethernetutil.h"

bool ETHERNET_PROCESSED = false;

void processEthernetSendQueue(EthernetDevice* device) {
    ETHERNET_PROCESSED = true;
}

void initializeEthernet(EthernetDevice* device) {
    initializeEthernetCommon(device);
}
