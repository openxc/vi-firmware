#ifndef __CANUTIL_LPC17XX__
#define __CANUTIL_LPC17XX__

#include "lpc17xx_can.h"

// TODO somewhere earlier we need to catch if the bus address is invalid, e.g. 0
// or 3
#define CAN_CONTROLLER(bus) ((LPC_CAN_TypeDef*)(bus->address == 1 ? LPC_CAN1 : LPC_CAN2))

#endif // __CANUTIL_LPC17XX__
