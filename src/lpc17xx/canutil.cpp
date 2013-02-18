#include "canutil.h"
#include "canutil_lpc17xx.h"
#include "signals.h"
#include "log.h"
#include "lpc17xx_pinsel.h"

// Same for both Blueboard and Ford CAN translator prototype
// CAN1: select P0.21 as RD1. P0.22 as TD1
// CAN2: select P0.4 as RD2, P0.5 as RD2
#define CAN_RX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 21 : 4)
#define CAN_TX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 22 : 5)
#define CAN_PORT_NUM(BUS) 0
#define CAN_FUNCNUM(BUS) (BUS == LPC_CAN1 ? 3 : 2)

CAN_ERROR configureFilters(CanBus* bus, CanFilter* filters, int filterCount) {
    if(filterCount > 0) {
        debugNoNewline("Configuring %d filters...", filterCount);
        CAN_SetAFMode(LPC_CANAF, CAN_Normal);
        CAN_ERROR result = CAN_OK;
        for(int i = 0; i < filterCount; i++) {
            result = CAN_LoadExplicitEntry(CAN_CONTROLLER(bus), filters[i].value,
                    STD_ID_FORMAT);
            if(result != CAN_OK) {
                debug("Couldn't add message filter, error %d", result);
            }
        }
        debug("Done.");
        return result;
    } else {
        debug("No filters configured, turning off acceptance filter");
        // disable acceptable filter so we get all messages
        CAN_SetAFMode(LPC_CANAF, CAN_AccBP);
        return CAN_OK;
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

bool CAN_CONTROLLER_INITIALIZED = false;

void initializeCan(CanBus* bus) {
    configureCanControllerPins(CAN_CONTROLLER(bus));
    configureTransceiver();

    if(!CAN_CONTROLLER_INITIALIZED) {
        // TODO workaround the fact that CAN_Init erase the acceptance filter
        // table, so we need to initialize both CAN controllers before setting
        // any filters, and then make sure not to call CAN_Init again.
        for(int i = 0; i < getCanBusCount(); i++) {
            CAN_Init(CAN_CONTROLLER((&getCanBuses()[i])), getCanBuses()[i].speed);
        }
        CAN_CONTROLLER_INITIALIZED = true;
    }
    CAN_ModeConfig(CAN_CONTROLLER(bus), CAN_OPERATING_MODE, ENABLE);

    // enable receiver interrupt
    CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_RIE, ENABLE);
    // enable transmit interrupt
    // TODO handle this?
    //CAN_IRQCmd(CAN_CONTROLLER(bus), CANINT_TIE1, ENABLE);

    NVIC_EnableIRQ(CAN_IRQn);

    int filterCount;
    CanFilter* filters = initializeFilters(bus->address, &filterCount);
    CAN_ERROR result = configureFilters(bus, filters, filterCount);
    if (result != CAN_OK) {
        debug("Unable to initialize CAN acceptance filters");
    }
}
