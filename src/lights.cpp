#include "timer.h"
#include "lights.h"
#include "listener.h"
#include "log.h"

Palette COLORS = {
    {255, 0, 0}, // red
    {0, 255, 0}, // green
    {0, 0, 255}, // blue
    {0, 0, 0}, // black (i.e. off)
    {255, 255, 255}, // white
};

void disable(Light light, int duration) {
    enable(light, COLORS.black, duration);
}

void disable(Light light) {
    enable(light, COLORS.black, 0);
}

void enable(Light light, RGB color, int duration) {
    enable(light, color);
    // TODO dim up
}

void flash(Light light, RGB color, int duration) {
    enable(light, color);
    // TODO make a non-blocking version of this using timers
    delayMs(duration);
    disable(light);
}

// TODO this doesn't exactly feel like it belongs here, maybe throw it in
// cantranslator.cpp for now. but we do want to change the USB connect status
// even with the emulator, so it would have to go in main.cpp.
void updateLightsCommon(Listener* listener) {
    // TODO we never set configured to false, so once you connect it will always
    // be green. I struggled with this a bit before but could we look at
    // detecting a disconnect again?
    if(listener->usb->configured) {
        enable(LIGHT_A, COLORS.green);
    } else {
        enable(LIGHT_A, COLORS.red);
    }

    // TODO update light_a based on CAN traffic
    // need a global status variable to reflects if we think the can bus is
    // active or not

    // TODO if we are going into low power, turn lights off (or will they go off
    // anyway?)
}
