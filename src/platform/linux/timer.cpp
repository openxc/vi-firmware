//#include "LPC17xx.h"
//#include "lpc_types.h"
//#include "lpc17xx_timer.h"
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "util/timer.h"

#define DELAY_TIMER LPC_TIM0

//unsigned int SYSTEM_TICK_COUNT;

/*extern "C" {

void SysTick_Handler() {
    ++SYSTEM_TICK_COUNT;
}

}*/

void openxc::util::time::delayMs(unsigned long delayInMs) {
	usleep(delayInMs * 1000);
//    TIM_TIMERCFG_Type delayTimerConfig;
//    TIM_ConfigStructInit(TIM_TIMER_MODE, &delayTimerConfig);
//    TIM_Init(DELAY_TIMER, TIM_TIMER_MODE, &delayTimerConfig);

//    DELAY_TIMER->PR  = 0x00;        /* set prescaler to zero */
//    DELAY_TIMER->MR0 = (SystemCoreClock / 4) / (1000 / delayInMs);  //enter delay time
//    DELAY_TIMER->IR  = 0xff;        /* reset all interrupts */
//    DELAY_TIMER->MCR = 0x04;        /* stop timer on match */

//    TIM_Cmd(DELAY_TIMER, ENABLE);

    /* wait until delay time has elapsed */
//    while (DELAY_TIMER->TCR & 0x01); */
}

unsigned long currentTimeInMillis() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // Works on Linux

    long s  = ts.tv_sec;
    time_t ms = round(ts.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

    return s * 1000 + ms;
}

unsigned long openxc::util::time::systemTimeMs() {
    return currentTimeInMillis();
}

void openxc::util::time::initialize() {
    // Configure for 1ms tick
//    SysTick_Config(SystemCoreClock / 1000);
}
