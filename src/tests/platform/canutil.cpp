#include "can/canutil.h"

bool openxc::can::addAcceptanceFilter(CanBus* bus, uint32_t id) {
    return true;
}

void openxc::can::removeAcceptanceFilter(CanBus* bus, uint32_t id) {
}

bool openxc::can::setAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    return true;
}
