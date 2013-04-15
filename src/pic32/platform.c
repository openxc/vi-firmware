#include "platform.h"

void initializePlatform() {
    // this is required to initialize the Wiring library from MPIDE's toolchain
    // (inspired by Arduino)
    init();
}
