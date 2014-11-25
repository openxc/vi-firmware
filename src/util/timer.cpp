#include <stddef.h>
#include <stdlib.h>
#include "util/log.h"
#include "util/timer.h"

#define MS_PER_SECOND 1000

unsigned long openxc::util::time::startupTimeMs() {
    static unsigned long startupTime = systemTimeMs();
    return startupTime;
}

unsigned long openxc::util::time::uptimeMs() {
    return systemTimeMs() - startupTimeMs();
}

/* Private: Return the period in ms given the frequency in hertz.
 */
static float frequencyToPeriod(float frequency) {
    return 1 / frequency * MS_PER_SECOND;
}

bool openxc::util::time::conditionalTick(FrequencyClock* clock) {
    return conditionalTick(clock, false);
}

static bool started(openxc::util::time::FrequencyClock* clock) {
    return clock->lastTick != 0;
}


static openxc::util::time::TimeFunction getTimeFunction(
        const openxc::util::time::FrequencyClock* clock) {
   return clock->timeFunction != NULL ? clock->timeFunction :
       openxc::util::time::systemTimeMs;
}

bool openxc::util::time::elapsed(FrequencyClock* clock, bool stagger) {
    if(clock == NULL) {
        return true;
    }

    float period = frequencyToPeriod(clock->frequency);
    float elapsedTime = 0;
    if(!started(clock) && stagger) {
        clock->lastTick = getTimeFunction(clock)() - (rand() % int(period));
    } else {
        // Make sure it ticks the the first call to conditionalTick(...)
        elapsedTime = !started(clock) ? period :
                getTimeFunction(clock)() - clock->lastTick;
    }

    return clock->frequency == 0 || elapsedTime >= period;
}

void openxc::util::time::tick(FrequencyClock* clock) {
    clock->lastTick = getTimeFunction(clock)();
}

bool openxc::util::time::conditionalTick(FrequencyClock* clock, bool stagger) {
    bool tick = elapsed(clock, stagger);
    if(tick) {
        clock->lastTick = getTimeFunction(clock)();
    }

    return tick;
}

void openxc::util::time::initializeClock(FrequencyClock* clock) {
    clock->lastTick = 0;
    clock->frequency = 0;
    clock->timeFunction = systemTimeMs;
}
