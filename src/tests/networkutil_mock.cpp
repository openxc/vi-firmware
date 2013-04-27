#include "interface/network.h"

bool NETWORK_PROCESSED = false;

void openxc::interface::network::processNetworkSendQueue(NetworkDevice* device) {
    NETWORK_PROCESSED = true;
}

void openxc::interface::network::initializeNetwork(NetworkDevice* device) {
    initializeNetworkCommon(device);
}
