#ifndef __RTCPIC_H_
#define __RTCPIC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    uint64_t tm;
    uint32_t isr_unix_time[2];
}ts;


extern volatile ts syst;
BOOL RTC_Init(void);
BOOL RTC_SetTimeUnix(uint32_t unixtime);
BOOL RTC_GetTimeDateDecimal(struct tm * ts);
void RTC_IsrTimeVarUpdate(void);
void rtc_task(void);
uint32_t RTC_GetTimeDateUnix(void);
void rtc_timer_ms_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
