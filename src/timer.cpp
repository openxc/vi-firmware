#include <stddef.h>
#include <stdlib.h>
#include "util/log.h"
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
    return shouldTick(clock, false);
}

static bool started(openxc::util::time::FrequencyClock* clock) {
    return clock->lastTick != 0;
}

bool openxc::util::time::shouldTick(FrequencyClock* clock, bool stagger) {
    if(clock == NULL) {
        return true;
    }

    unsigned long (*timeFunction)() =
            clock->timeFunction != NULL ? clock->timeFunction : systemTimeMs;
    float elapsedTime = 0;
    float period = frequencyToPeriod(clock->frequency);
    if(!started(clock) && stagger) {
        clock->lastTick = timeFunction() - (rand() % int(period));
    } else {
        // Make sure it ticks the the first call to shouldTick
        elapsedTime = !started(clock) ? period :
                timeFunction() - clock->lastTick;
    }

    bool tick = false;
    if(clock->frequency == 0 || elapsedTime >= period) {
        clock->lastTick = timeFunction();
        tick = true;
    }

    return tick;
}

void openxc::util::time::initializeClock(FrequencyClock* clock) {
    clock->lastTick = 0;
    clock->frequency = 0;
    clock->timeFunction = systemTimeMs;
}
