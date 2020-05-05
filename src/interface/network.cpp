#include "interface/network.h"

#include <stddef.h>

#include "util/log.h"
#include "config.h"

using openxc::util::log::debug;

void openxc::interface::network::initializeCommon(NetworkDevice* device) {
    if(device != NULL) {
        debug("Initializing Network...");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
        device->descriptor.type = InterfaceType::NETWORK;
    }
}

size_t openxc::interface::network::handleIncomingMessage(uint8_t payload[], size_t length) {
    return commands::handleIncomingMessage(payload, length,
            &config::getConfiguration()->network.descriptor);
}

bool openxc::interface::network::connected(NetworkDevice* device) {
    return device != NULL && device->configured;
}
