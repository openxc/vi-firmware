#include "serialutil.h"
#include "log.h"
#include <stddef.h>

const int openxc::interface::uart::MAX_MESSAGE_SIZE = 128;

void openxc::interface::uart::initializeSerialCommon(SerialDevice* device) {
    if(device != NULL) {
        debug("Initializing UART.....");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
    }
}
