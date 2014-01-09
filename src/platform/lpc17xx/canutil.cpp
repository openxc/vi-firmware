#include "can/canutil.h"
#include "canutil_lpc17xx.h"
#include "signals.h"
#include "util/log.h"
#include "lpc17xx_pinsel.h"

// Same for both Blueboard and Ford VI prototype
// CAN1: select P0.21 as RD1. P0.22 as TD1
// CAN2: select P0.4 as RD2, P0.5 as RD2
#define CAN_RX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 21 : 4)
#define CAN_TX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 22 : 5)
#define CAN_PORT_NUM(BUS) 0
#define CAN_FUNCNUM(BUS) (BUS == LPC_CAN1 ? 3 : 2)

using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;
using openxc::util::log::debug;

bool openxc::can::setAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    // TODO are there different controls for each bus? documentation indicates
    // no, the AF mode is global
    if(enabled) {
        CAN_SetAFMode(LPC_CANAF, CAN_Normal);
    } else {
        CAN_SetAFMode(LPC_CANAF, CAN_AccBP);
    }
    return true;
}

bool openxc::can::addAcceptanceFilter(CanBus* bus, uint32_t id) {
    // TODO for a diagnostic request, when does a filter get removed? if a
    // request is completed and no other active requsts have the same id
    setAcceptanceFilterStatus(bus, true);

    for(int i = 0; i < sizeof(bus->acceptanceFilters); i++) {
        AcceptanceFilter* filter = bus->acceptanceFilters[i];
        if(filter->active && filter->id == id) {
            return true;
        }
    }

    // it's not already in there, see if we have space
    AcceptanceFilter* availableFilter = NULL;
    for(int i = 0; i < sizeof(bus->acceptanceFilters); i++) {
        AcceptanceFilter* filter = bus->acceptanceFilters[i];
        if(!filter->active) {
            availableFilter = filter;
            break;
        }
    }

    if(availableFilter == NULL) {
        debug("All acceptance filter slots already taken, can't add 0x%lx",
                id);
        return false;
    }

    // TODO everything in this function except this is portable - we need an
    // addAcceptanceFilter (in canutil.cpp and addAcceptanceFilterPlatformSpecific or something
    // like that (in platform/*/canutil.cpp)
    if(CAN_LoadExplicitEntry(CAN_CONTROLLER(bus), id, STD_ID_FORMAT) != CAN_OK) {
        debug("Couldn't add message filter for ID 0x%x", id);
        // put it back in the free list
        LIST_INSERT_HEAD(&bus->freeAcceptanceFilters, newEntry, entries);
        return false;
    }

    newEntry->filter = id;
    LIST_INSERT_HEAD(&bus->acceptanceFilters, newEntry, entries);
    return true;
}

void openxc::can::removeAcceptanceFilter(CanBus* bus, uint32_t id) {
    AcceptanceFilterListEntry* entry;
    for(entry = bus->acceptanceFilters.lh_first; entry != NULL;
            entry = entry->entries.le_next) {
        if(entry->filter == id) {
            break;
        }
    }

    if(entry != NULL) {
        LIST_REMOVE(entry, entries);
        // TODO actually remove from CAN controller
        // there is a CAN_RemoveEntry, but it requires a "position" argument
        // much like the PIC32's API. how are we supposed to know the position
        // if we added it with LoadExplicitEntry, and that didn't give us any
        // position back? we have to keep track as we add them and hope the
        // internal counts are the same?
    }

    if(bus->acceptanceFilters.lh_first == NULL) {
        // when all filters are removed, switch into bypass mode
        setAcceptanceFilterStatus(bus, false);
    }
}

void configureCanControllerPins(LPC_CAN_TypeDef* controller) {
    PINSEL_CFG_Type PinCfg;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Funcnum = CAN_FUNCNUM(controller);
    PinCfg.Pinnum = CAN_RX_PIN_NUM(controller);
    PinCfg.Portnum = CAN_PORT_NUM(controller);
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.Pinnum = CAN_TX_PIN_NUM(controller);
    PINSEL_ConfigPin(&PinCfg);
}

void configureTransceiver() {
    // make P0.19 high to make sure the TJ1048T is awake
    LPC_GPIO0->FIODIR |= 1 << 19;
    LPC_GPIO0->FIOPIN |= 1 << 19;
    LPC_GPIO0->FIODIR |= 1 << 6;
    LPC_GPIO0->FIOPIN |= 1 << 6;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable) {
    can::initializeCommon(bus);
    configureCanControllerPins(CAN_CONTROLLER(bus));
    configureTransceiver();

    static bool CAN_CONTROLLER_INITIALIZED = false;
    // TODO workaround the fact that CAN_Init erase the acceptance filter
    // table, so we need to initialize both CAN controllers before setting
    // any filters, and then make sure not to call CAN_Init again.
    if(!CAN_CONTROLLER_INITIALIZED) {
        for(int i = 0; i < getCanBusCount(); i++) {
            CAN_Init(CAN_CONTROLLER((&getCanBuses()[i])), getCanBuses()[i].speed);
        }
        CAN_CONTROLLER_INITIALIZED = true;
    }

    CAN_MODE_Type mode = CAN_LISTENONLY_MODE;
    if(writable) {
        debug("Initializing bus %d in writable mode", bus->address);
        mode = CAN_OPERATING_MODE;
    } else {
        debug("Initializing bus %d in listen only mode", bus->address);
    }
    CAN_ModeConfig(CAN_CONTROLLER(bus), mode, ENABLE);

    // enable receiver interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_RIE, ENABLE);
    // enable transmit interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_TIE1, ENABLE);

    NVIC_EnableIRQ(CAN_IRQn);

    if(!configureDefaultFilters(bus, openxc::signals::getMessages(),
            openxc::signals::getMessageCount())) {
        debug("Unable to initialize CAN acceptance filters");
    }
}
