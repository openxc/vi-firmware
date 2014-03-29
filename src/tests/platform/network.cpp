#include "interface/network.h"
#include "commands.h"

using openxc::commands::IncomingMessageCallback;

bool NETWORK_PROCESSED = false;

void openxc::interface::network::processSendQueue(NetworkDevice* device) {
    NETWORK_PROCESSED = true;
}

void openxc::interface::network::initialize(NetworkDevice* device) {
    network::initializeCommon(device);
}

void openxc::interface::network::read(NetworkDevice* device,
        IncomingMessageCallback callback) { }
