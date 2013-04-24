#include "networkutil.h"

bool NETWORK_PROCESSED = false;

void openxc::network::processNetworkSendQueue(NetworkDevice* device) {
    NETWORK_PROCESSED = true;
}

void openxc::network::initializeNetwork(NetworkDevice* device) {
    initializeNetworkCommon(device);
}
