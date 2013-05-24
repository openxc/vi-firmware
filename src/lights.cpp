#include "util/timer.h"
#include "lights.h"
#include "interface/pipeline.h"
#include "util/log.h"

using openxc::util::time::delayMs;

openxc::lights::Palette openxc::lights::COLORS = {
    {255, 0, 0}, // red
    {0, 255, 0}, // green
    {0, 0, 255}, // blue
    {0, 0, 0}, // black (i.e. off)
    {255, 255, 255}, // white
};

void openxc::lights::disable(Light light, int duration) {
    enable(light, COLORS.black, duration);
}

void openxc::lights::disable(Light light) {
    enable(light, COLORS.black, 0);
}

void openxc::lights::enable(Light light, RGB color, int duration) {
    enable(light, color);
    // TODO dim up
}

void openxc::lights::flash(Light light, RGB color, int duration) {
    enable(light, color);
    // TODO make a non-blocking version of this using timers
    delayMs(duration);
    disable(light);
}

void openxc::lights::deinitialize() {
    disable(LIGHT_A);
    disable(LIGHT_B);
}
