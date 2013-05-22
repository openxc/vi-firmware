#include "interface/network.h"

void openxc::interface::network::initializeNetwork(NetworkDevice* uart) { }

void openxc::interface::network::processNetworkSendQueue(NetworkDevice* device) { }

void openxc::interface::network::readFromSocket(NetworkDevice* device, bool (*callback)(uint8_t*)) { }
