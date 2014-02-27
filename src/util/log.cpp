#include "util/log.h"
#include "pipeline.h"
#include <stdio.h>
#include <stdarg.h>

#define LOG_QUEUE_FLUSH_MAX_TRIES 5

const int openxc::util::log::MAX_LOG_LINE_LENGTH = 256;

namespace usb = openxc::interface::usb;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::interface::usb::UsbDevice;
using openxc::pipeline::Pipeline;
using openxc::pipeline::MessageClass;

extern Pipeline PIPELINE;

void openxc::util::log::debug(const char* format, ...) {
#ifdef __DEBUG__
    va_list args;
    va_start(args, format);

    char buffer[MAX_LOG_LINE_LENGTH];
    vsnprintf(buffer, MAX_LOG_LINE_LENGTH - 2, format, args);
    strncat(buffer, "\r\n", 2);

    pipeline::sendMessage(&PIPELINE, (uint8_t*) buffer,
            strnlen(buffer, MAX_LOG_LINE_LENGTH), MessageClass::LOG);

    va_end(args);
#endif // __DEBUG__
}
