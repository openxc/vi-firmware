#include "interface/uart.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include <cstddef>

using openxc::interface::uart::UartDevice;

bool UART_PROCESSED = false;

void openxc::interface::uart::processUartSendQueue(UartDevice* device) {
    UART_PROCESSED = true;
}

void openxc::interface::uart::readFromUart(UartDevice* uart, bool (*callback)(uint8_t*)) { }

void openxc::interface::uart::initializeUart(UartDevice* uart) {
    initializeUartCommon(uart);
}

bool openxc::interface::uart::uartConnected(UartDevice* device) {
    return device != NULL;
}

