#include "canutil_spy.h"

static bool _acceptanceFilterStatus = true;

bool openxc::can::spy::getAcceptanceFilterStatus() {
    return _acceptanceFilterStatus;
}

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    return true;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    initializeCommon(bus);
}

bool openxc::can::resetAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    _acceptanceFilterStatus = enabled;
    return true;
}
