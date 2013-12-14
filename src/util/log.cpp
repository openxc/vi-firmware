#include "util/log.h"
#include "interface/usb.h"
#include <stdio.h>
#include <stdarg.h>

const int openxc::util::log::MAX_LOG_LINE_LENGTH = 120;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::interface::usb::UsbDevice;

extern UsbDevice USB_DEVICE;

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

#ifdef __UART_LOGGING__
	debugUart(buffer);
#endif // __UART_LOGGING__

    conditionalEnqueue(
            &USB_DEVICE.endpoints[LOG_ENDPOINT_INDEX].sendQueue,
            (uint8_t*) buffer, strnlen(buffer, MAX_LOG_LINE_LENGTH));

    va_end(args);
#endif // __DEBUG__
}
