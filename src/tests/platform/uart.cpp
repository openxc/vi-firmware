#include "interface/uart.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include <cstddef>

using openxc::interface::uart::UartDevice;

bool UART_PROCESSED = false;

void openxc::interface::uart::processSendQueue(UartDevice* device) {
    UART_PROCESSED = true;
}

void openxc::interface::uart::read(UartDevice* uart, bool (*callback)(uint8_t*)) { }

void openxc::interface::uart::initialize(UartDevice* uart) {
    uart::initializeCommon(uart);
}

bool openxc::interface::uart::connected(UartDevice* device) {
    return device != NULL;
}

