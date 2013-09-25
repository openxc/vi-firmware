#include <stddef.h>
#include "util/timer.h"

#define MS_PER_SECOND 1000

unsigned long openxc::util::time::startupTimeMs() {
    static unsigned long startupTime = time::systemTimeMs();
    return startupTime;
}

unsigned long openxc::util::time::uptimeMs() {
    return systemTimeMs() - startupTimeMs();
}

/* Private: Return the period in ms given the frequency in hertz.
 */
float frequencyToPeriod(float frequency) {
    return 1 / frequency * MS_PER_SECOND;
}

bool openxc::util::time::shouldTick(FrequencyClock* clock) {
    if(clock == NULL) {
        return true;
    }

    unsigned long (*timeFunction)() =
            clock->timeFunction != NULL ? clock->timeFunction : systemTimeMs;
    float elapsedTime = timeFunction() - clock->lastTick;
    bool tick = false;
    if(!clock->started || clock->frequency == 0 ||
            elapsedTime >= frequencyToPeriod(clock->frequency)) {
        tick = true;
        clock->lastTick = timeFunction();
        clock->started = true;
    }
    return tick;
}

void openxc::util::time::initializeClock(FrequencyClock* clock) {
    clock->lastTick = -1;
    clock->frequency = 0;
    clock->timeFunction = systemTimeMs;
    clock->started = false;
}
