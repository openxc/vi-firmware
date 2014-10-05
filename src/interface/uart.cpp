#include "uart.h"

#include <stddef.h>

#include "util/log.h"
#include "config.h"

const int openxc::interface::uart::MAX_MESSAGE_SIZE = 128;

using openxc::util::log::debug;

void openxc::interface::uart::initializeCommon(UartDevice* device) {
    if(device != NULL) {
        debug("Initializing UART.....");
        QUEUE_INIT(uint8_t, &device->receiveQueue);
        QUEUE_INIT(uint8_t, &device->sendQueue);

        device->descriptor.type = InterfaceType::UART;
    }
}

size_t openxc::interface::uart::handleIncomingMessage(uint8_t payload[], size_t length) {
    return openxc::commands::handleIncomingMessage(payload, length,
            &config::getConfiguration()->uart.descriptor);
}
