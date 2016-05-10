#include "platform_profile.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <plib.h>
#include "WProgram.h"
#include "rtcc.h"
#include "rtc.h"

volatile ts syst;

static uint32_t last_time_check;
void RTC_IsrTimeVarUpdate(void);
static uint8_t timer_deinit = 0;
#ifdef RTC_SUPPORT
void __ISR(_TIMER_2_VECTOR,ipl2) _TIMER2_HANDLER(void) 
{
    syst.isr_unix_time[0]++;
    if(syst.isr_unix_time[0] == 0) //overflow
        syst.isr_unix_time[1]++;
    mT2ClearIntFlag();
}

void RTC_InitTimer(void){
    //Generate a 1ms timer event
    OpenTimer2( T2_ON | T2_SOURCE_INT | T2_PS_1_256, 313);
    ConfigIntTimer2( T2_INT_ON | T2_INT_PRIOR_2);
}

void RTC_IsrTimeVarUpdate(void) //Call this regularly somewhere ever hour or so
{
    syst.tm = (uint64_t)RTCCGetTimeDateUnix()*1000;

}

BOOL RTC_Init(void){

    if(I2C_Initialize() != RTCC_NO_ERROR){
        __debug("RTC I2C Init Error");
    }
    
	last_time_check = 0;
	
    RTC_IsrTimeVarUpdate();

    RTC_InitTimer();
	
	timer_deinit = 0;
    return true;
}

BOOL RTC_SetTimeUnix(uint32_t unixtime){
  
    __debug("Set RTC Time %x", unixtime);
	RTC_IsrTimeVarUpdate();
    return (RTCCSetTimeDateUnix(unixtime) == RTCC_NO_ERROR)? true: false;
    return 0;

}

BOOL RTC_GetTimeDateDecimal(struct tm * ts){
      
    if(RTCCGetTimeDateDecimal(ts) != RTCC_NO_ERROR){
        __debug("RTC Read Error");
    }

    return true;
}

uint32_t RTC_GetTimeDateUnix(void){    
    RTC_IsrTimeVarUpdate();
    return (syst.tm/1000);
}

void rtc_task(void){
    
    if((syst.isr_unix_time[0] - last_time_check) > RTC_UPDATE_INT_MS)
    {
		last_time_check = syst.isr_unix_time[0];
        RTC_IsrTimeVarUpdate();
    }
}

void rtc_timer_ms_deinit(void){
    T2CON = 0x0;
    TMR2 = 0x0;    //stop timer
    ConfigIntTimer2( T2_INT_OFF); //disable interrupts
	timer_deinit = 1;
}
#endif