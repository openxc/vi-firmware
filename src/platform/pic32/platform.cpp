#include "platform/platform.h"
#include "WProgram.h"

void openxc::platform::initialize() {
    // this is required to initialize the Wiring library from MPIDE's toolchain
    // (inspired by Arduino)
    init();
}
