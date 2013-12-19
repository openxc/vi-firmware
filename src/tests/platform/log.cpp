#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>

void openxc::util::log::debugUart(const char* message) {
    printf(message);
}

void openxc::util::log::initialize() { }
