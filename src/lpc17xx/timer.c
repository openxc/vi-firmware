#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_timer.h"
#include "timer.h"

#define SYSTEM_CLOCK_TIMER LPC_TIM3
#define DELAY_TIMER LPC_TIM0

void TIMER3_IRQHandler() { }

void resetTimer(LPC_TIM_TypeDef* timer) {
    timer->TCR = 2;
}

void delayMs(int delayInMs) {
    resetTimer(DELAY_TIMER);
    DELAY_TIMER->PR  = 0x00;        /* set prescaler to zero */
    DELAY_TIMER->MR0 = (SystemCoreClock / 4) / (1000 / delayInMs);  //enter delay time
    DELAY_TIMER->IR  = 0xff;        /* reset all interrrupts */
    DELAY_TIMER->MCR = 0x04;        /* stop timer on match */
    TIM_Cmd(DELAY_TIMER, ENABLE);

    /* wait until delay time has elapsed */
    while (DELAY_TIMER->TCR & 0x01);
}

unsigned long systemTimeMs() {
    return SYSTEM_CLOCK_TIMER->TC;
}

void initializeTimers() {
    TIM_TIMERCFG_Type systemClockTimerConfig;
    TIM_ConfigStructInit(TIM_TIMER_MODE, &systemClockTimerConfig);
    TIM_Init(SYSTEM_CLOCK_TIMER, TIM_TIMER_MODE, &systemClockTimerConfig);
    TIM_Cmd(SYSTEM_CLOCK_TIMER, ENABLE);
}
