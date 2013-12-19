#include "util/log.h"
#include "interface/usb.h"
#include "pipeline.h"
#include <stdio.h>
#include <stdarg.h>

#define LOG_QUEUE_FLUSH_MAX_TRIES 5

const int openxc::util::log::MAX_LOG_LINE_LENGTH = 120;

namespace usb = openxc::interface::usb;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::interface::usb::UsbDevice;
using openxc::pipeline::Pipeline;

extern UsbDevice USB_DEVICE;
extern Pipeline PIPELINE;

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

#ifdef __UART_LOGGING__
	debugUart(buffer);
#endif // __UART_LOGGING__

#ifndef __TESTS__
    // flush the USB queue so we don't lose log messages - the outgoing queue is
    // fairly small
    int timeout = LOG_QUEUE_FLUSH_MAX_TRIES;
    while(timeout > 0 && !conditionalEnqueue(
            &USB_DEVICE.endpoints[LOG_ENDPOINT_INDEX].sendQueue,
            (uint8_t*) buffer, strnlen(buffer, MAX_LOG_LINE_LENGTH))) {
        usb::processSendQueue(PIPELINE.usb);
        --timeout;
    }
#endif // __TESTS__

    va_end(args);
#endif // __DEBUG__
}
