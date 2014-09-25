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

extern uint16_t CANAF_std_cnt;
extern uint16_t CANAF_ext_cnt;

static void configureCanControllerPins(LPC_CAN_TypeDef* controller) {
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

static void configureTransceiver() {
    // make P0.19 high to make sure the TJ1048T is awake
    LPC_GPIO0->FIODIR |= 1 << 19;
    LPC_GPIO0->FIOPIN |= 1 << 19;
    LPC_GPIO0->FIODIR |= 1 << 6;
    LPC_GPIO0->FIOPIN |= 1 << 6;
}

static void clearAcceptanceFilterTable() {
    // remove all existing entries - I tried looping over CAN_RemoveEntry until
    // it returned an error, but that left the AF table in a corrupted state.
    CAN_SetAFMode(LPC_CANAF, CAN_AccOff);

    CANAF_std_cnt = 0;
    CANAF_ext_cnt = 0;
    memset((void*)LPC_CANAF_RAM->mask, 0, 512);

    LPC_CANAF->SFF_sa = 0x00;
    LPC_CANAF->SFF_GRP_sa = 0x00;
    LPC_CANAF->EFF_sa = 0x00;
    LPC_CANAF->EFF_GRP_sa = 0x00;
    LPC_CANAF->ENDofTable = 0x00;
}

bool openxc::can::resetAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    CAN_SetAFMode(LPC_CANAF, enabled ? CAN_Normal : CAN_AccBP);
    return true;
}

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    clearAcceptanceFilterTable();

    uint16_t filterCount = 0;
    CAN_ERROR result = CAN_OK;
    bool bypassFilters = false;
    for(int i = 0; i < busCount; i++) {
        CanBus* bus = &buses[i];
        bypassFilters |= bus->bypassFilters;
        AcceptanceFilterListEntry* entry;
        LIST_FOREACH(entry, &bus->acceptanceFilters, entries) {
            if(filterCount >= MAX_ACCEPTANCE_FILTERS) {
                break;
            }
            result = CAN_LoadExplicitEntry(CAN_CONTROLLER(bus), entry->filter,
                   entry->format == CanMessageFormat::STANDARD ?
                       STD_ID_FORMAT : EXT_ID_FORMAT);
           if(result != CAN_OK) {
                debug("Couldn't add filter 0x%x to bus %d", entry->filter,
                        bus->address);
                break;
           }
           ++filterCount;
        }
    }

    // On the LPC17xx, the AF mode is global - if it's off, it's off for
    // both controllers. That's why this is outside the loop above, and
    // we're counting *total* filters, not filters per bus. We also disable the
    // AF if any of the busses has bypassFilters == true.
    bypassFilters |= filterCount == 0;
    if(bypassFilters) {
        debug("No filters configured or a bus in bypass, disabling AF");
    }
    resetAcceptanceFilterStatus(NULL, !bypassFilters);
    return result == CAN_OK;
}

void openxc::can::deinitialize(CanBus* bus) { }

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    can::initializeCommon(bus);
    configureCanControllerPins(CAN_CONTROLLER(bus));
    configureTransceiver();

    // Be aware that CAN_Init erase the acceptance filter table for BOTH
    // controllers, so we need to initialize both CAN controllers before setting
    // any filters, and then make sure to re-add the filters if we call CAN_Init
    // again. Here we initialize both buses when the first bus is initialized,
    // to get it out of the way.
    static bool CAN_CONTROLLER_INITIALIZED = false;
    if(!CAN_CONTROLLER_INITIALIZED) {
        for(int i = 0; i < getCanBusCount(); i++) {
            debug("Initializing bus %d at %d baud", getCanBuses()[i].address,
                    getCanBuses()[i].speed);
            CAN_Init(CAN_CONTROLLER((&getCanBuses()[i])), getCanBuses()[i].speed);
        }
        CAN_CONTROLLER_INITIALIZED = true;
    }

    CAN_MODE_Type mode = CAN_LISTENONLY_MODE;
    if(bus->loopback) {
        debug("Initializing bus %d in loopback mode", bus->address);
        mode = CAN_SELFTEST_MODE;
    } else if(writable) {
        debug("Initializing bus %d in writable mode", bus->address);
        mode = CAN_OPERATING_MODE;
    } else {
        debug("Initializing bus %d in listen only mode", bus->address);
    }
    CAN_ModeConfig(CAN_CONTROLLER(bus), mode, ENABLE);

    if(!configureDefaultFilters(bus, openxc::signals::getMessages(),
            openxc::signals::getMessageCount(), buses, busCount)) {
        debug("Unable to initialize CAN acceptance filters");
    }

    // enable receiver interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_RIE, ENABLE);
    // enable transmit interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_TIE1, ENABLE);

    NVIC_EnableIRQ(CAN_IRQn);
}
