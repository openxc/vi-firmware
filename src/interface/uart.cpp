#include "interface/uart.h"
#include "util/log.h"
#include <stddef.h>

const int openxc::interface::uart::MAX_MESSAGE_SIZE = 128;

using openxc::util::log::debug;

void openxc::interface::uart::initializeCommon(UartDevice* device) {
    if(device != NULL) {
        debug("Initializing UART.....");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);

        device->allowRawWrites =
#ifdef UART_ALLOW_RAW_WRITE
            true
#else
            false
#endif
            ;
    }
}
