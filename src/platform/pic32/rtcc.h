#ifndef _RTCC_DOT_H_
#define _RTCC_DOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "GenericTypeDefs.h"
#include "platform_profile.h"
#include <stdbool.h>
#include <stdint.h>
#define I2C_DATABUFFER_SIZE             32

#define RTCC_I2C_BAUD                   100000

#define I2C_BIT_TIME_MICROSECONDS       (1000000 / RTCC_I2C_BAUD)

#define I2C_INIT_MILLISECONDS_DELAY     200

#define I2C_PINFLOAT_MILLISECONDS_DELAY 100

#define I2C_MAX_MILLISECONDS_EVENT      100

#define EEPROM_WRITECYCLE_MILLISECONDS  10

/*ENUMERATIONS*/

typedef enum {

    RTCC_MEMTYPE_SRAM = 0xD0,
    RTCC_MEMTYPE_EEPROM = 0xA0
    
}RTCC_MEMTYPE;

typedef enum {

    RTCC_MEMOP_WRITE = 0x0E,
    RTCC_MEMOP_READ = 0x0F

}RTCC_MEMOP;

typedef enum {
    EEPROM_WRITE = 0xAE,
    EEPROM_READ = 0xAF,
    SRAM_WRITE = 0xDE,
    SRAM_READ = 0xDF
}RTCC_CONTROLWORDS;

typedef enum {

    RTCC_TIMEDATE_STARTADDR = 0x00,
    RTCC_UNIQUEID_STARTADDR = 0xF0,
    RTCC_ALARM0_STARTADDR = 0x0A,
    RTCC_CONTROLREGISTER_STARTADDR = 0x07,
    RTCC_POWERDOWN_STARTADDR = 0x18,
    RTCC_POWERUP_STARTADDR = 0x1C,
    RTCC_EVENTLOGTIMEDATE_STARTADDR = 0x08

}RTCC_ADDRESSES;

typedef enum {

    RTCC_ALARM_SECONDS = 0,
    RTCC_ALARM_MINUTES = 1,
    RTCC_ALARM_HOURS = 2,
    RTCC_ALARM_DAY = 3,
    RTCC_ALARM_DATE = 4,
    RTCC_ALARM_TIMEDATE = 7

}RTCC_ALARMTYPES;

typedef enum {
    RTCC_NO_ERROR,
    RTCC_MEMTYPE_INVALID,
    RTCC_SRAM_ADDRESS_OOR,
    RTCC_EEPROM_ADDRESS_OOR,
    RTCC_BYTECOUNT_OOR,
    RTCC_I2C_INIT_MODULE_OFF_TIMEOUT,
    RTCC_I2C_INIT_MODULE_ON_TIMEOUT,
    RTCC_I2C_DISABLE_TIMEOUT,
    RTCC_I2C_SENDSTART_TIMEOUT,
    RTCC_I2C_SENDRESTART_TIMEOUT,
    RTCC_I2C_SENDBYTE_TIMEOUT,
    RTCC_I2C_RECEIVEBYTE_TIMEOUT,
    RTCC_I2C_ACK_TIMEOUT,
    RTCC_I2C_NACK_TIMEOUT,
    RTCC_I2C_SENDSTOP_TIMEOUT,
    RTCC_I2C_TRANSMIT_NOACK,
    RTCC_I2C_RESET_FAILED,
    RTCC_I2C_START_COLLISION,
    RTCC_I2C_RESTART_FAILED,
    RTCC_I2C_STOP_FAILED,
    RTCC_I2C_RECEIVE_FAILED,
    RTCC_I2C_ACK_FAILED,
    RTCC_I2C_NACK_FAILED,
    RTCC_I2C_SEND_FAILED
}RTCC_STATUS;

/*MACROS*/

#define RTCC_ST_MASK     0x80

/*STRUCTS*/
typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t oscillator_enable;
    uint8_t battery_enable;
    char C5ID[7];
}RTC_TIMEDATESETTINGS;

typedef struct {
    uint8_t data[I2C_DATABUFFER_SIZE];
    uint8_t * pData;
}I2CDataBuffer;

/*FUNCTIONS*/

/*initialize I2C to RTCC*/
RTCC_STATUS I2C_Initialize();
RTCC_STATUS I2C_Disable();

/*generic RTCC memory read/write functions*/
RTCC_STATUS RTCC_ReadBytes(RTCC_MEMTYPE memtype, uint8_t address, uint8_t length, uint8_t * data);
RTCC_STATUS RTCC_WriteBytes(RTCC_MEMTYPE memtype, uint8_t address, uint8_t length, uint8_t * data);
RTCC_STATUS RTCC_Unlock();

/*higher level RTCC memory functions*/
RTCC_STATUS RTCCGetC5ID(uint8_t pC5ID[7]);
RTCC_STATUS RTCCSetC5ID(uint8_t pC5ID[7]);

RTCC_STATUS RTCCSetTimeDateBCD(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t year, uint8_t oscillator_enable, uint8_t battery_enable);
RTCC_STATUS RTCCSetTimeDateDecimal(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t year, uint8_t oscillator_enable, uint8_t battery_enable);
RTCC_STATUS RTCCSetTimeDateUnix(time_t UnixTime);
RTCC_STATUS RTCCGetTimeDateBCD(struct tm * TimeDate);
RTCC_STATUS RTCCGetTimeDateDecimal(struct tm * TimeDate);
RTCC_STATUS RTCCSetAlarmBCD(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t alarmType);
RTCC_STATUS RTCCSetAlarmDecimal(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t alarmType);
RTCC_STATUS RTCCGetPowerDownTimestampBCD(struct tm * TimeDate);
RTCC_STATUS RTCCGetPowerDownTimestampDecimal(struct tm * TimeDate);
RTCC_STATUS RTCCGetPowerUpTimestampBCD(struct tm * TimeDate);
RTCC_STATUS RTCCGetPowerUpTimestampDecimal(struct tm * TimeDate);
RTCC_STATUS RTCCClearTimestamps(void);
RTCC_STATUS RTCCSetCalibrationRegister(INT8 reg_val);
RTCC_STATUS RTCCGetCrystalState(bool *crystal_state);
RTCC_STATUS RTCCSetCrystalState(bool crystal_state);
time_t ConvertRealTimeToUnixTime(struct tm *RealTime);
struct tm * ConvertUnixTimeToRealTime(time_t * UnixTime);
time_t RTCCGetTimeDateUnix(void);

#ifdef __cplusplus
}
#endif

#endif