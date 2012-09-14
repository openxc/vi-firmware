#include "canutil.h"
#include "signals.h"
#include "log.h"
#include "lpc17xx_can.h"
#include "lpc17xx_pinsel.h"

#define CAN_CTRL(BUS) (BUS == LPC_CAN1 ? 0 : 1)

// CAN1: select P0.0 as RD1. P0.1 as TD1
// CAN2: select P2.7 as RD2, P2.8 as RD2
#define CAN_RX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 0 : 7)
#define CAN_TX_PIN_NUM(BUS) (BUS == LPC_CAN1 ? 1 : 8)
#define CAN_PORT_NUM(BUS) (BUS == LPC_CAN1 ? 0 : 2)

CAN_ERROR configureFilters(CanBus* bus, CanFilter* filters, int filterCount) {
    debug("Configuring %d filters...", filterCount);
    CAN_ERROR result = CAN_OK;
    for(int i = 0; i < filterCount; i++) {
        result = CAN_LoadFullCANEntry(bus->controller, filters[i].value);
    }
    debug("Done.\r\n");
    return result;
}

void configureCanControllerPins(LPC_CAN_TypeDef* controller) {
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = CAN_RX_PIN_NUM(controller);
    PinCfg.Portnum = CAN_PORT_NUM(controller);
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.Pinnum = CAN_TX_PIN_NUM(controller);
    PINSEL_ConfigPin(&PinCfg);
}

void initializeCan(CanBus* bus) {
    QUEUE_INIT(CanMessage, &bus->receiveQueue);
    QUEUE_INIT(CanMessage, &bus->sendQueue);

    configureCanControllerPins(bus->controller);

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
