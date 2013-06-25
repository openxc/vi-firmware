#include "interface/uart.h"
#include "util/log.h"
#include <stddef.h>

const int openxc::interface::uart::BAUD_RATE = 230000;
const int openxc::interface::uart::MAX_MESSAGE_SIZE = 128;

using openxc::util::log::debugNoNewline;

void openxc::interface::uart::initializeCommon(UartDevice* device) {
    if(device != NULL) {
        debugNoNewline("Initializing UART.....");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);
    }
}
