#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_timer.h"
#include "timer.h"

#define SYSTEM_CLOCK_TIMER LPC_TIM3
#define DELAY_TIMER LPC_TIM0

void delayMs(int delayInMs) {
    TIM_TIMERCFG_Type delayTimerConfig;
    TIM_ConfigStructInit(TIM_TIMER_MODE, &delayTimerConfig);
    TIM_Init(DELAY_TIMER, TIM_TIMER_MODE, &delayTimerConfig);

    DELAY_TIMER->PR  = 0x00;        /* set prescaler to zero */
    DELAY_TIMER->MR0 = (SystemCoreClock / 4) / (1000 / delayInMs);  //enter delay time
    DELAY_TIMER->IR  = 0xff;        /* reset all interrupts */
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
    systemClockTimerConfig.PrescaleOption = TIM_PRESCALE_TICKVAL;
    systemClockTimerConfig.PrescaleValue = SystemCoreClock / (4 * 1000);
    TIM_Init(SYSTEM_CLOCK_TIMER, TIM_TIMER_MODE, &systemClockTimerConfig);
    TIM_Cmd(SYSTEM_CLOCK_TIMER, ENABLE);
}
