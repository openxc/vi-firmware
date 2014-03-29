#include "can/canutil.h"

bool ACCEPTANCE_FILTER_STATUS = true;

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    return ACCEPTANCE_FILTER_STATUS;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    initializeCommon(bus);
}
