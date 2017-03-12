#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/if.h>
#include <linux/can.h>
#include <thread>
#include "can/canutil.h"
#include "signals.h"
#include "util/log.h"
#include "platform/linux/canread.h"

using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;
using openxc::util::log::debug;

bool openxc::can::resetAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    return true;
}

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    uint16_t filterCount = 0;
    bool result = true;
    bool bypassFilters = false;
    for(int i = 0; i < busCount; i++) {
        CanBus* bus = &buses[i];
        bypassFilters |= bus->bypassFilters;
        AcceptanceFilterListEntry* entry;
        LIST_FOREACH(entry, &bus->acceptanceFilters, entries) {
           ++filterCount;
        }
    }

    bypassFilters |= filterCount == 0;
    if(bypassFilters) {
        debug("No filters configured or a bus in bypass, disabling AF");
    }
    resetAcceptanceFilterStatus(NULL, !bypassFilters);
    return result;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
		const int busCount) {
	can::initializeCommon(bus);
	
	if (!configureDefaultFilters(bus, openxc::signals::getMessages(),
			openxc::signals::getMessageCount(), buses, busCount)) {
        	debug("Unable to initialize CAN acceptance filters");
	}

	struct sockaddr_can addr;
	struct ifreq ifr;

    	int socketId = socket(PF_CAN, SOCK_RAW, CAN_RAW);

	char* canDeviceName = "vcan0";
	debug("CAN device name: " + bus->address);
	strcpy(ifr.ifr_name,  canDeviceName);
	ioctl(socketId, SIOCGIFINDEX, &ifr);

	debug("Interface name: " + ifr.ifr_ifindex);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	bind(socketId, (struct sockaddr *)&addr, sizeof(addr));

	std::thread* monitor = new std::thread(canReaderHandler, bus, socketId);
}
