#include "power.h"
#include "log.h"
#include "gpio.h"
#include <stdbool.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"

#define POWER_CONTROL_PORT 2
#define POWER_CONTROL_PIN 13

uint32_t DISABLED_PERIPHERALS[] = {
    CLKPWR_PCONP_PCTIM0,
    CLKPWR_PCONP_PCTIM1,
    CLKPWR_PCONP_PCSPI,
    CLKPWR_PCONP_PCI2C0,
    CLKPWR_PCONP_PCRTC,
    CLKPWR_PCONP_PCSSP1,
    CLKPWR_PCONP_PCI2C1,
    CLKPWR_PCONP_PCSSP0,
    CLKPWR_PCONP_PCI2C2,
};

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
    debug("Initializing power controls...");
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

    debugNoNewline("Turning off unused peripherals...");
    for(int i = 0; i < sizeof(DISABLED_PERIPHERALS) /
            sizeof(DISABLED_PERIPHERALS[0]); i++) {
        CLKPWR_ConfigPPWR(DISABLED_PERIPHERALS[i], DISABLE);
    }

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

    // Disable brown-out detection when we go into lower power
    LPC_SC->PCON |= (1 << 2);

    // Disable all the things
    LPC_SC->PCONP = 0;

    CLKPWR_PowerDown();
}
