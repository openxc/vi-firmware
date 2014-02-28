#include "can/canutil.h"

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    return true;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    initializeCommon(bus);
}
