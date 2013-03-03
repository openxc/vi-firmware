#include "power.h"
#include "log.h"
#include "gpio.h"
#include <stdbool.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_wdt.h"

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

void WDT_IRQHandler(void) {
    // Disable WDT interrupt
    NVIC_DisableIRQ(WDT_IRQn);
    WDT_ClrTimeOutFlag();
}

void CANActivity_IRQHandler(void) {
    NVIC_DisableIRQ(CANActivity_IRQn);
    LPC_SC->CANSLEEPCLR = (0x1<<1)|(0x1<<2);
    LPC_CAN1->MOD = LPC_CAN2->MOD &= ~(0x1<<4);
    LPC_SC->CANWAKEFLAGS = (0x1<<1)|(0x1<<2);
}

#define WDT_TIMEOUT     2000000

void enterLowPowerMode() {
    debug("Going to low power mode");
    NVIC_EnableIRQ(CANActivity_IRQn);
    WDT_Init(WDT_CLKSRC_IRC, WDT_MODE_INT_ONLY);
    /* NVIC_EnableIRQ(WDT_IRQn); */
    WDT_Start(WDT_TIMEOUT);
    LPC_SC->PLL0CON &= ~(1<<1); /* Disconnect the main PLL (PLL0) */
    LPC_SC->PLL0FEED = 0xAA; /* Feed */
    LPC_SC->PLL0FEED = 0x55; /* Feed */
    while ((LPC_SC->PLL0STAT & (1<<25)) != 0x00); /* Wait for main PLL (PLL0) to disconnect */
    LPC_SC->PLL0CON &= ~(1<<0); /* Turn off the main PLL (PLL0) */
    LPC_SC->PLL0FEED = 0xAA; /* Feed */
    LPC_SC->PLL0FEED = 0x55; /* Feed */
    while ((LPC_SC->PLL0STAT & (1<<24)) != 0x00); /* Wait for main PLL (PLL0) to shut down */
    CLKPWR_PowerDown();

    NVIC_SystemReset();
    /* SystemInit(); */
    /* initializeLogging(); */

    /* debug("Woke up from low power mode"); */
    /* debug("SMFLAG: %d, DSFLAG: %d, PDFLAG: %d", */
            /* LPC_SC->PCON & (1 << 8), LPC_SC->PCON & (1 << 9), LPC_SC->PCON & (1 << 10)); */
}
