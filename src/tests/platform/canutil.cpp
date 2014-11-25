#include "canutil_spy.h"

static bool _acceptanceFiltersUpdated = false;

bool openxc::can::spy::acceptanceFiltersUpdated() {
    return _acceptanceFiltersUpdated;
}

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    _acceptanceFiltersUpdated = true;
    return true;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    initializeCommon(bus);
}

bool openxc::can::resetAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    _acceptanceFiltersUpdated = false;
    return true;
}
