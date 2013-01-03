#include "serialutil.h"
#include "log.h"
#include <stddef.h>

void initializeSerialCommon(SerialDevice* device) {
    if(device != NULL) {
        debug("Initializing UART.....");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
    }
}
