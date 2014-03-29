#include "power.h"
#include "util/log.h"
#include "gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_wdt.h"

#define POWER_CONTROL_PORT 2
#define POWER_CONTROL_PIN 13

#define PROGRAM_BUTTON_PORT 2
#define PROGRAM_BUTTON_PIN 12

namespace gpio = openxc::gpio;

using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::util::log::debug;

const uint32_t DISABLED_PERIPHERALS[] = {
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
    debug("Switching 12v power passthrough ");
    if(enabled) {
        debug("on");
        pinStatus = 0;
    } else {
        debug("off");
        pinStatus = 1;
    }
    gpio::setValue(POWER_CONTROL_PORT, POWER_CONTROL_PIN,
            pinStatus ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
}

void openxc::power::initialize() {
    // Configure 12v passthrough control as a digital output
    PINSEL_CFG_Type powerPassthroughPinConfig;
    powerPassthroughPinConfig.OpenDrain = 0;
    powerPassthroughPinConfig.Pinmode = 1;
    powerPassthroughPinConfig.Funcnum = 0;
    powerPassthroughPinConfig.Portnum = POWER_CONTROL_PORT;
    powerPassthroughPinConfig.Pinnum = POWER_CONTROL_PIN;
    PINSEL_ConfigPin(&powerPassthroughPinConfig);

    gpio::setDirection(POWER_CONTROL_PORT, POWER_CONTROL_PIN, GPIO_DIRECTION_OUTPUT);
    setPowerPassthroughStatus(true);

    debug("Turning off unused peripherals");
    for(unsigned int i = 0; i < sizeof(DISABLED_PERIPHERALS) /
            sizeof(DISABLED_PERIPHERALS[0]); i++) {
        CLKPWR_ConfigPPWR(DISABLED_PERIPHERALS[i], DISABLE);
    }

    PINSEL_CFG_Type programButtonPinConfig;
    programButtonPinConfig.OpenDrain = 0;
    programButtonPinConfig.Pinmode = 1;
    programButtonPinConfig.Funcnum = 1;
    programButtonPinConfig.Portnum = PROGRAM_BUTTON_PORT;
    programButtonPinConfig.Pinnum = PROGRAM_BUTTON_PIN;
    PINSEL_ConfigPin(&programButtonPinConfig);
}

void openxc::power::handleWake() {
    // This isn't especially graceful, we just reset the device after a
    // wakeup. Then again, it makes the code a hell of a lot simpler because we
    // only have to worry about initialization of core peripherals in one spot,
    // setup() in vi_firmware.cpp and main.cpp. I'll leave this for now and we
    // can revisit it if there is some reason we need to keep state between CAN
    // wakeup events (e.g. fine_odometer_since_restart could be more persistant,
    // but then it actually might be more confusing since it'd be
    // fine_odometer_since_we_lost_power)
    NVIC_SystemReset();
}

void openxc::power::suspend() {
    debug("Going to low power mode");
    NVIC_EnableIRQ(CANActivity_IRQn);
    NVIC_EnableIRQ(EINT2_IRQn);

    setPowerPassthroughStatus(false);

    // Disable brown-out detection when we go into lower power
    LPC_SC->PCON |= (1 << 2);

    // TODO do we need to disable and disconnect the main PLL0 before ending
    // deep sleep, accoridn gto errata lpc1768-16.march2010? it's in some
    // example code from NXP.
    CLKPWR_DeepSleep();
}

void openxc::power::enableWatchdogTimer(int microseconds) {
    WDT_Init(WDT_CLKSRC_IRC, WDT_MODE_RESET);
    WDT_Start(microseconds);
}

void openxc::power::disableWatchdogTimer() {
    // TODO this is nuts, but you can't change the WDMOD register until after
    // the WDT times out. But...we are using a RESET with the WDT, so the whole
    // board will reset and then we don't have any idea if the WDT should be
    // disabled or not! This makes it really difficult to use the WDT for both
    // normal runtime hard freeze protection and periodic wakeup from sleep (to
    // check if CAN is active via OBD-II). I have to disable the regular WDT for
    // now for this reason.
    LPC_WDT->WDMOD = 0x0;
}

void openxc::power::feedWatchdog() {
    WDT_Feed();
}

extern "C" {

void CANActivity_IRQHandler(void) {
    openxc::power::handleWake();
}

void EINT2_IRQHandler(void) {
    openxc::power::handleWake();
}

}
