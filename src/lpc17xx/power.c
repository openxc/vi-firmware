#include "power.h"
#include "log.h"
#include "gpio.h"
#include <stdbool.h>
#include "lpc17xx_pinsel.h"

#define POWER_CONTROL_PORT 2
#define POWER_CONTROL_PIN 13

void setPowerPassthroughStatus(bool enabled) {
    int pinStatus;
    debugNoNewline("Switching 12v power passthrough ");
    if(enabled) {
        debug("on");
        pinStatus = 0;
    } else {
        debug("off");
        pinStatus = 1;
    }
    setGpioValue(POWER_CONTROL_PORT, POWER_CONTROL_PIN, pinStatus);
}

void initializePower() {
    debugNoNewline("Initializing power controls...");
    // Configure 12v passthrough control as a digital output
    PINSEL_CFG_Type PinCfg;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 1;
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = POWER_CONTROL_PORT;
    PinCfg.Pinnum = POWER_CONTROL_PIN;
    PINSEL_ConfigPin(&PinCfg);

    setGpioDirection(POWER_CONTROL_PORT, POWER_CONTROL_PIN, GPIO_DIRECTION_OUTPUT);
    setPowerPassthroughStatus(false);

    debug("Done.");
}

void updatePower() {
}
