#include "LPC17xx.h"
#include "lpc_types.h"
#include "timer.h"

#define SYSTEM_CLOCK_TIMER LPC_TIM3
#define DELAY_TIMER LPC_TIM0

void resetTimer(LPC_TIM_TypeDef* timer) {
    timer->TCR = 2;
}

void startTimer(LPC_TIM_TypeDef* timer) {
    timer->TCR = 1;
}

void setTimerMode(LPC_TIM_TypeDef* timer) {
    timer->CTCR = 0;
}

void delayMs(int delayInMs) {
    resetTimer(DELAY_TIMER);
    DELAY_TIMER->PR  = 0x00;        /* set prescaler to zero */
    DELAY_TIMER->MR0 = (SystemCoreClock / 4) / (1000 / delayInMs);  //enter delay time
    DELAY_TIMER->IR  = 0xff;        /* reset all interrrupts */
    DELAY_TIMER->MCR = 0x04;        /* stop timer on match */
    startTimer(DELAY_TIMER);

    /* wait until delay time has elapsed */
    while (LPC_TIM0->TCR & 0x01);
}

unsigned long systemTimeMs() {
    return SYSTEM_CLOCK_TIMER->TC;
}

void initializeTimers() {
    resetTimer(SYSTEM_CLOCK_TIMER);
    setTimerMode(SYSTEM_CLOCK_TIMER);
    SYSTEM_CLOCK_TIMER->PR = 24 - 1; // microsecond resolution
    startTimer(SYSTEM_CLOCK_TIMER);
}
