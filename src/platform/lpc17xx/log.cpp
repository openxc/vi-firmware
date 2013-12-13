#include "util/log.h"
#include "interface/usb.h"
#include <stdio.h>
#include <stdarg.h>
#include "interface/usb.h"

extern "C" {
#include "debug_frmwrk.h"
}

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::interface::usb::UsbDevice;

extern UsbDevice USB_DEVICE;

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    _printf(buffer);

    // TODO
    conditionalEnqueue(
            &USB_DEVICE.endpoints[LOG_ENDPOINT_NUMBER - 1].sendQueue,
            (uint8_t*) buffer, strnlen(buffer, MAX_LOG_LINE_LENGTH));

    va_end(args);
#endif // __DEBUG__
}

void openxc::util::log::initialize() {
    debug_frmwrk_init();
}
