#include "util/timer.h"

unsigned long openxc::util::time::startupTimeMs() {
    static unsigned long startupTime = time::systemTimeMs();
    return startupTime;
}

unsigned long openxc::util::time::uptimeMs() {
    return systemTimeMs() - startupTimeMs();
}
