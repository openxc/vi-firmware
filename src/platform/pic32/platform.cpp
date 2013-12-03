#include "platform/platform.h"
#include "WProgram.h"

extern "C" {
extern void __use_isr_install(void);
__attribute__((section(".comment"))) void (*__use_force_isr_install)(void) = &__use_isr_install;
}

void openxc::platform::initialize() {
    // this is required to initialize the Wiring library from MPIDE's toolchain
    // (inspired by Arduino)
    init();
}
