#include "interface/network.h"

void openxc::interface::network::initialize(NetworkDevice* uart) { }

void openxc::interface::network::processSendQueue(NetworkDevice* device) { }

void openxc::interface::network::read(NetworkDevice* device, bool (*callback)(uint8_t*)) { }
