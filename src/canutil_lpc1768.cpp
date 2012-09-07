#ifdef __LPC17XX__

#include "canutil.h"
#include "signals.h"
#include "log.h"
#include "lpc17xx_can.h"
#include "lpc17xx_pinsel.h"

#define CAN_CTRL(BUS) (BUS == LPC_CAN1 ? 0 : 1)

CAN_ERROR configureFilters(CanBus* bus, CanFilter* filters, int filterCount) {
    debug("Configuring %d filters...", filterCount);
    CAN_ERROR result = CAN_OK;
    for(int i = 0; i < filterCount; i++) {
        result = CAN_LoadFullCANEntry(bus->controller, filters[i].value);
    }
    debug("Done.");
    return result;
}

void initializeCan(CanBus* bus) {
    queue_init(&bus->sendQueue);

    // TODO need to store pin configurations in the struct
    /* Pin configuration
     * CAN1: select P0.0 as RD1. P0.1 as TD1
     */
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 1;
    PINSEL_ConfigPin(&PinCfg);

    // the bus is coming up as 0 in the struct for some reason
    CAN_Init(bus->controller, bus->speed);
    CAN_ModeConfig(bus->controller, CAN_OPERATING_MODE, ENABLE);

    // enable receiver interrupt
    CAN_IRQCmd(bus->controller, CANINT_RIE, ENABLE);
    // enable transmit interrupt
    // TODO handle this?
    //CAN_IRQCmd(bus->controller, CANINT_TIE1, ENABLE);

    NVIC_EnableIRQ(CAN_IRQn);
    // configure acceptance filter in bypass mode
    CAN_SetAFMode(LPC_CANAF, CAN_Normal);

    int filterCount;
    CanFilter* filters = initializeFilters(bus->address, &filterCount);
    CAN_ERROR result = configureFilters(bus, filters, filterCount);
    if (result != CAN_OK) {
        debug("Unable to initialize CAN acceptance filters");
    }
}

void CAN_IRQHandler() {
    if((CAN_IntGetStatus(LPC_CAN1) >> 0) & 0x1) {
        getCanBuses()[0].messageReceived = true;
    } else if((CAN_IntGetStatus(LPC_CAN2) >> 0) & 0x1) {
        getCanBuses()[1].messageReceived = true;
    }
}

#endif // __LPC17XX__
