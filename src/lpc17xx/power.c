#include "power.h"
#include "log.h"
#include "gpio.h"
#include <stdbool.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"

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

void CANActivity_IRQHandler(void) {
    // TODO This isn't especially graceful, we just reset the device after a
    // wakeup. Then again, it makes the code a hell of a lot simpler because we
    // only have to worry about initialization of core peripherals in one spot,
    // setup() in cantranslator.cpp and main.cpp. I'll leave this for now and we
    // can revisit it if there is some reason we need to keep state between CAN
    // wakeup events (e.g. fine_odometer_since_restart could be more persistant,
    // but then it actually might be more confusing since it'd be
    // fine_odometer_since_we_lost_power)
    NVIC_SystemReset();
}

void enterLowPowerMode() {
    debug("Going to low power mode");
    NVIC_EnableIRQ(CANActivity_IRQn);

    CLKPWR_PowerDown();
}
