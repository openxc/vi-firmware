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

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    // remove all existing entries - I tried looping over CAN_RemoveEntry until
    // it returned an error, but that left the AF table in a corrupted state.
    LPC_CANAF->AFMR = 0x00000001;
    CANAF_std_cnt = 0;
    for (int i = 0; i < 512; i++) {
        LPC_CANAF_RAM->mask[i] = 0x00;
    }

    LPC_CANAF->SFF_sa = 0x00;
    LPC_CANAF->SFF_GRP_sa = 0x00;
    LPC_CANAF->EFF_sa = 0x00;
    LPC_CANAF->EFF_GRP_sa = 0x00;
    LPC_CANAF->ENDofTable = 0x00;

    uint16_t filterCount = 0;
    CAN_ERROR result = CAN_OK;
    for(int i = 0; i < busCount; i++) {
        CanBus* bus = &buses[i];
        for(const AcceptanceFilterListEntry* entry = bus->acceptanceFilters.lh_first;
                entry != NULL && filterCount < MAX_ACCEPTANCE_FILTERS;
                entry = entry->entries.le_next, ++filterCount) {
           result = CAN_LoadExplicitEntry(CAN_CONTROLLER(bus), entry->filter,
                                       STD_ID_FORMAT);
           if(result != CAN_OK) {
                debug("Couldn't add filter 0x%x to bus %d", entry->filter,
                        bus->address);
                break;
           }
        }
    }

    if(filterCount == 0) {
        debug("No filters configured, turning off acceptance filter");
        // TODO this is an issue on LPC17xx when the AF is a global setting -
        // we can't have it off for one bus and on for the other
        CAN_SetAFMode(LPC_CANAF, CAN_AccBP);
    }

    return result == CAN_OK;
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

void openxc::can::initialize(CanBus* buses, const int busCount, CanBus* bus,
        bool writable) {
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

    if(!configureDefaultFilters(buses, busCount, bus,
            openxc::signals::getMessages(),
            openxc::signals::getMessageCount())) {
        debug("Unable to initialize CAN acceptance filters");
    }

    // enable receiver interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_RIE, ENABLE);
    // enable transmit interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_TIE1, ENABLE);

    NVIC_EnableIRQ(CAN_IRQn);
}
