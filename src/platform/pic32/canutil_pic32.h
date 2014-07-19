#ifndef __CANUTIL__PIC32__
#define __CANUTIL__PIC32__

#include "chipKITCAN.h"

// Defined in pic32/canutil.cpp - we don't want to put a function in the generic
// canutil.h module because the type of the controller is different for each
// platform, e.g. CAN vs LPC_CAN. We could have a function that returns a void,
// but then you have to cast it everywhere back to the actual type.
extern CAN* can1;
extern CAN* can2;

#define SYS_FREQ (80000000L)
#define CAN_CONTROLLER(bus) ((CAN*)(bus->address == 1 ? can1 : can2))

namespace openxc {
namespace can {
namespace pic32 {

void handleCanInterrupt(CanBus* bus);

}
}
}

#endif // __CANUTIL__PIC32__
