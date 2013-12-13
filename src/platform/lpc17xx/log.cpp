#include "util/log.h"
#include "interface/usb.h"
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include "debug_frmwrk.h"
}

using openxc::util::bytebuffer::conditionalEnqueue;

void openxc::util::log::debugNoNewline(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    _printf(buffer);

    // TOOD
    conditionalEnqueue(
            &pipeline->usb->endpoints[LOG_ENDPOINT_NUMBER - 1].sendQueue,
            buffer, strnlen(buffer, MAX_LOG_LINE_LENGTH));

    va_end(args);
#endif // __DEBUG__
}

void openxc::util::log::initialize() {
    debug_frmwrk_init();
}
