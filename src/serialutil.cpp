#include "serialutil.h"
#include "log.h"

void initializeSerialCommon(SerialDevice* device) {
    debug("Initializing UART.....");
    QUEUE_INIT(uint8_t, &device->receiveQueue);
    QUEUE_INIT(uint8_t, &device->sendQueue);
}
