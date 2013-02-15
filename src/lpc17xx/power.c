#include "power.h"
#include "log.h"
#include <stdbool.h>
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

#define POWER_CONTROL_PORT 2
#define POWER_CONTROL_PIN 13

#define INPUT 0
#define OUTPUT 1

void gpioSetDirection(uint32_t port, uint32_t pin, uint32_t direction) {
    uint32_t bitmask = 1 << pin;
    GPIO_SetDir(port, bitmask, direction);
}

void gpioSetValue(uint32_t port, uint32_t pin, uint32_t value) {
    uint32_t bitmask = 1 << pin;
    if(value == 0) {
        GPIO_ClearValue(port, bitmask);
    } else {
        GPIO_SetValue(port, bitmask);
    }
}

void setPowerPassthroughStatus(bool enabled) {
    int pinStatus;
    debug("Switching 12v power passthrough ");
    if(enabled) {
        debug("on");
        pinStatus = 0;
    } else {
        debug("off");
        pinStatus = 1;
    }
    debug("\r\n");
    gpioSetValue(POWER_CONTROL_PORT, POWER_CONTROL_PIN, pinStatus);
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

    gpioSetDirection(POWER_CONTROL_PORT, POWER_CONTROL_PIN, OUTPUT);
    setPowerPassthroughStatus(false);

    debug("Done.\r\n");
}

void updatePower() {
}
