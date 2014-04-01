#include "util/log.h"
#include "pipeline.h"
#include <stdio.h>
#include "config.h"
#include <stdarg.h>

#define LOG_QUEUE_FLUSH_MAX_TRIES 5

const int openxc::util::log::MAX_LOG_LINE_LENGTH = 256;

namespace usb = openxc::interface::usb;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::interface::usb::UsbDevice;
using openxc::pipeline::Pipeline;
using openxc::pipeline::MessageClass;
using openxc::config::getConfiguration;

void openxc::util::log::debug(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH, format, args);

    // Send strlen + 1 so we make sure to include the NULL character as a
    // delimiter
    pipeline::sendMessage(&getConfiguration()->pipeline, (uint8_t*) buffer,
            strnlen(buffer, MAX_LOG_LINE_LENGTH) + 1, MessageClass::LOG);

    va_end(args);
#endif // __DEBUG__
}
