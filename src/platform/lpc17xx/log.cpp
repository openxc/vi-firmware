#include "util/log.h"

extern "C" {
#include "debug_frmwrk.h"
}

void openxc::util::log::initialize() {
    debug_frmwrk_init();
}

void openxc::util::log::debugUart(const char* message) {
    _printf(message);
}
