// Compiler includes
#include <p32xxxx.h>
#include <plib.h>
#include <string.h>
#include <time.h>
#include "rtcc.h"
#include "WProgram.h" 
#include "platform_profile.h"

/*PRIVATE VARIABLES*/
I2CDataBuffer I2CRxBuffer;

#ifdef RTC_SUPPORT


/*PRIVATE FUNCTION PROTOTYPES*/

RTCC_STATUS I2C_Reset(void);
RTCC_STATUS I2C_Start(void);
RTCC_STATUS I2C_Restart(void);
RTCC_STATUS I2C_Stop(void);
RTCC_STATUS I2C_Acknowledge_ACK(void);
RTCC_STATUS I2C_Acknowledge_NACK(void);
RTCC_STATUS I2C_Receive(void);
RTCC_STATUS I2C_Transmit(uint8_t data);

/*PUBLIC FUNCTIONS*/


static void delay_ms(uint32_t d)
{
    uint32_t tm;
    
    tm = millis() + d;            //todo create a microsecond differential coretick timer delay
    
    while(millis() < tm);        //todo if millis does not increase will result in halt
    
}

RTCC_STATUS I2C_Initialize() {

    // return code
    RTCC_STATUS stat;

    // disable I2C module
    I2CEnable(RTCC_I2C_MODULE, FALSE);

    // release I2C pins and give them time to float
    BUS_RELEASE();
    
    delay_ms(I2C_INIT_MILLISECONDS_DELAY);

    stat = RTCC_I2C_INIT_MODULE_OFF_TIMEOUT;
    
    // check the bus condition
    if(SDA_STATE == 0 || SCL_STATE == 0)
    {
        // if the slave is driving the bus low, we need to call I2C_Reset()
        __debug("I2C bus fault");
        stat = I2C_Reset();
        if(stat != RTCC_NO_ERROR)
        {
            __debug("RTC Init Failed SDA/SCL default low");
            goto fcn_exit;
        }
    }
    
    // enable the I2C module
    I2C_CONFIGURATION conf = (I2C_STOP_IN_IDLE);
    I2CConfigure(RTCC_I2C_MODULE, conf);
    I2CEnable(RTCC_I2C_MODULE, TRUE);
    
    delay_ms(I2C_INIT_MILLISECONDS_DELAY);
    
    // set the baud rate generator
    I2CSetFrequency(RTCC_I2C_MODULE, vCPU_CLOCK_HZ, RTCC_I2C_BAUD);

    stat = RTCC_NO_ERROR;

    fcn_exit:

    // if the function failed, we should disable the peripheral before returning
    if(stat != RTCC_NO_ERROR)
    {
        I2CEnable(RTCC_I2C_MODULE, FALSE);
    }
    return stat;
    
}

RTCC_STATUS I2C_Disable() {
    I2CEnable(RTCC_I2C_MODULE, FALSE);
    return RTCC_NO_ERROR;
}

// abstract
RTCC_STATUS RTCC_ReadBytes(RTCC_MEMTYPE memtype, uint8_t address, uint8_t length, uint8_t *data) {

    /*
     * RTCC I2C Read Sequence
     * 0) Validate request
     *      RTCC SRAM size 32 bytes, address range 0x00 to 0x1F
     *      General SRAM size 64 bytes, address range 0x20 to 0x5F
     *      General EEPROM size 128 bytes, address range 0x00 to 0x7F
     *      UniqueID size 8 bytes, address range 0xF0 to 0xF7
     * 1) Send start condition
     * 2) Master transmission, control word memtype_WRITE
     * 3) Receive ACK to confirm master transmission
     * 4) Master transmission, AddressPointer
     * 5) Receive ACK to confirm master transmission
     * 6) Send repeated start to break current transaction
     * 7) Master transmission, control word memtype_READ
     * 8) Receive ACK to confirm master transmission
     * 9) Initiate master receive sequence
     * 10) Wait for master receive sequence to complete
     * 11) Read the I2C receive register
     * 12) Send ACK (initiate next byte transfer) or NACK (stop byte transfer)
     * 13) Repeat 9-12 until all bytes are read
     * 14) Send stop condition
     */

    // counter for sequential byte opeation
    uint8_t i;
    // return code
    RTCC_STATUS stat;

    // validate read address and read length
    if(memtype == RTCC_MEMTYPE_SRAM)
    {
        // RTCC block
        if(address <= 0x1F)
        {
            if(length == 0 || (address + length) > 0x20)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // general SRAM block
        else if(address <= 0x5F)
        {
            if(length == 0 || (address + length) > 0x60)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // SRAM address OOR
        else
        {
            stat = RTCC_SRAM_ADDRESS_OOR;
            goto fcn_exit;
        }
    }
    else if(memtype == RTCC_MEMTYPE_EEPROM)
    {
        // EEPROM block
        if(address <= 0x7F)
        {
            if(length == 0 || (address + length) > 0x80)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // UniqueID block
        else if(address <= 0xF7)
        {
            if(length == 0 || (address + length) > 0xF8)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // EEPROM address OOR
        else
        {
            stat = RTCC_EEPROM_ADDRESS_OOR;
            goto fcn_exit;
        }
    }
    else
    {
        stat = RTCC_MEMTYPE_INVALID;
        goto fcn_exit;
    }

    // initialize I2C receive buffer
    memset(I2CRxBuffer.data, 0x00, I2C_DATABUFFER_SIZE);
    I2CRxBuffer.pData = &I2CRxBuffer.data[0];

    // initialize byte loop counter
    i = 0;

    // send start condition
    stat = I2C_Start();
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // send control word for memory write
    stat = I2C_Transmit(memtype + RTCC_MEMOP_WRITE);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // send memory address
    stat = I2C_Transmit(address);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // send restart to reset transaction
    stat = I2C_Restart();
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // send control for memory read
    stat = I2C_Transmit(memtype + RTCC_MEMOP_READ);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // loop through sequential byte read
    while(i < length)
    {
        // increment byte loop counter
        i++;

        // initiate I2C master receive sequence
        stat = I2C_Receive();
        if(stat != RTCC_NO_ERROR) goto fcn_exit;

        // read and clear the I2C receive register
        // write to and advance the I2C receive buffer pointer
        //*(I2CRxBuffer.pData++) = I2C5RCV;
        *(I2CRxBuffer.pData++) = I2CGetByte(RTCC_I2C_MODULE);

        // set acknowledge bit to ACK to initiate next byte transfer
        if(i < length)
        {
            stat = I2C_Acknowledge_ACK();
            if(stat != RTCC_NO_ERROR) goto fcn_exit;
        }
        // set acknowledge bit to NACK to stop byte transfer
        else
        {
            stat = I2C_Acknowledge_NACK();
            if(stat != RTCC_NO_ERROR) goto fcn_exit;
        }
    }

    // send stop condition
    stat = I2C_Stop();
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // copy the I2CRxBuffer to the caller's data buffer
    memcpy(data, I2CRxBuffer.data, length);

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCC_WriteBytes(RTCC_MEMTYPE memtype, uint8_t address, uint8_t length, uint8_t * data) {

    /*
     * RTCC I2C Write Sequence
     * 0) Validate request
     *      RTCC SRAM size 32 bytes, address range 0x00 to 0x1F
     *      General SRAM size 64 bytes, address range 0x20 to 0x5F
     *      General EEPROM size 128 bytes, address range 0x00 to 0x7F
     *      UniqueID size 8 bytes, address range 0xF0 to 0xF7
     * 1) Send start condition
     * 2) Master transmission, 1 byte, memtype_WRITE
     * 3) Receive ACK to confirm master transmission
     * 4) Master transmission, 1 byte, WriteAddress
     * 5) Receive ACK to confirm master transmission
     * 6) Master transmission, 1 byte, DataByte0
     * 7) Receive ACK to confirm master transmission
     * 8) Repeat 6-7 for all data bytes 1 through n (up to page boundary for
     *    EEPROM writes)
     * 9) Send stop condition
     */

     // counter for sequential byte opeation
    uint8_t i;
    // return code
    RTCC_STATUS stat = RTCC_NO_ERROR;
    // write cycle delay time
    uint32_t writecycledelay;

    // validate read address and read length
    if(memtype == RTCC_MEMTYPE_SRAM)
    {
        // RTCC block
        if(address <= 0x1F)
        {
            if(length == 0 || (address + length) > 0x20)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // general SRAM block
        else if(address <= 0x5F)
        {
            if(length == 0 || (address + length) > 0x60)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // SRAM address OOR
        else
        {
            stat = RTCC_SRAM_ADDRESS_OOR;
            goto fcn_exit;
        }
    }
    else if(memtype == RTCC_MEMTYPE_EEPROM)
    {
        // EEPROM block
        if(address <= 0x7F)
        {
            if(length == 0 || (address + length) > 0x80)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // UniqueID block
        else if(address <= 0xF7)
        {
            if(length == 0 || (address + length) > 0xF8)
            {
                stat = RTCC_BYTECOUNT_OOR;
                goto fcn_exit;
            }
        }
        // EEPROM address OOR
        else
        {
            stat = RTCC_EEPROM_ADDRESS_OOR;
            goto fcn_exit;
        }
    }
    else
    {
        stat = RTCC_MEMTYPE_INVALID;
        goto fcn_exit;
    }

    // initialize byte loop counter
    i = 0;

    // write cycle loop
    // multiple write cycles are needed to write across EEPROM page boundaries
    // SRAM writes have no page boundaries and are done in a single write cycle
    while(i < length)
    {
        // send start condition
        stat = I2C_Start();
        if(stat != RTCC_NO_ERROR) goto fcn_exit;

        // send control word for memory write
        stat = I2C_Transmit(memtype + RTCC_MEMOP_WRITE);
        if(stat != RTCC_NO_ERROR) goto fcn_exit;

        // send memory address
        stat = I2C_Transmit(address + i);
        if(stat != RTCC_NO_ERROR) goto fcn_exit;

        // loop through sequential byte write
        for(; i < length; ++i)
        {
            // transmit data byte
            stat = I2C_Transmit(*(uint8_t *)(data + i));
            if(stat != RTCC_NO_ERROR) goto fcn_exit;

            // break out of this write cycle if we are writing to EEPROM
            // and the next write crosses a page boundary
            if(memtype == RTCC_MEMTYPE_EEPROM)
            {
                if(((address + i + 1) % 8) == 0)
                {
                    ++i;
                    break;
                }
            }

        }

        // send stop condition
        stat = I2C_Stop();
        if(stat != RTCC_NO_ERROR) goto fcn_exit;

        if(memtype == RTCC_MEMTYPE_EEPROM)
        {
            // EEPROM page write cycle delay
            delay_ms(EEPROM_WRITECYCLE_MILLISECONDS);
            
        }

    }

    // small delay so we don't step on ourselves
    delay_ms(1);
    __debug("status = %d", stat);
    fcn_exit:
    
    return stat;

}

RTCC_STATUS RTCC_Unlock() {

    // unlock code
    uint8_t unlockcode;
    // return code
    RTCC_STATUS stat = RTCC_NO_ERROR;

    unlockcode = 0x55;
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, 0x09, 1, &unlockcode);
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }
    
    delay_ms(5);

    unlockcode = 0xAA;
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, 0x09, 1, &unlockcode);
    if(stat != RTCC_NO_ERROR) 
    {
        goto fcn_exit;
    }

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetC5ID(uint8_t a_C5ID[7]) {

    // return code
    RTCC_STATUS stat = RTCC_NO_ERROR;

    // get the bytes
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_EEPROM, RTCC_UNIQUEID_STARTADDR, 6, a_C5ID);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // null terminate the string
    a_C5ID[6] = 0;

    fcn_exit:

    return stat;
}

RTCC_STATUS RTCCSetC5ID(uint8_t pC5ID[7]) {

    // return code
    RTCC_STATUS stat;

    // unlock
    stat = RTCC_Unlock();
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }
    
    delay_ms(5);

    // do a sequential write to the RTCC EEPROM, 6 bytes (null byte not written)
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_EEPROM, RTCC_UNIQUEID_STARTADDR, 6, pC5ID);
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }
    
    fcn_exit:
    
    return stat;

}

RTCC_STATUS RTCCSetTimeDateBCD(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t year, uint8_t oscillator_enable, uint8_t battery_enable) {

    // return code
    RTCC_STATUS stat;
    // packed data structure
    uint8_t TimeDateRegisters[7];

    // pack up the BCD time, date, and settings and send to the RTCC
    TimeDateRegisters[0] = second + ((oscillator_enable & 0x01) << 7);
    TimeDateRegisters[1] = minute;
    TimeDateRegisters[2] = hour;
    TimeDateRegisters[3] = (battery_enable & 0x01) << 3;
    TimeDateRegisters[4] = date;
    TimeDateRegisters[5] = month;
    TimeDateRegisters[6] = year;

    // send the packed bytes to the SRAM time, date, and configuration registers
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 7, TimeDateRegisters);

    return stat;

}

RTCC_STATUS RTCCSetTimeDateDecimal(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t year, uint8_t oscillator_enable, uint8_t battery_enable) {

    // return code
    RTCC_STATUS stat;
    // packed data structure
    uint8_t TimeDateRegisters[7];

    // convert the decimal values to BCD
    TimeDateRegisters[0] = (second % 10) + ((second/10)<<4) + ((oscillator_enable & 0x01) << 7);
    TimeDateRegisters[1] = (minute % 10) + ((minute/10)<<4);
    TimeDateRegisters[2] = (hour % 10) + ((hour/10)<<4);
    TimeDateRegisters[3] = (battery_enable & 0x01) << 3;
    TimeDateRegisters[4] = (date % 10) + ((date/10)<<4);
    TimeDateRegisters[5] = (month % 10) + ((month/10)<<4);
    year -= 2000;
    TimeDateRegisters[6] = (year % 10) + ((year/10)<<4);

    // send the BCD registers to the RTCC
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 7, TimeDateRegisters);

    return stat;
    
}

RTCC_STATUS RTCCSetTimeDateUnix(time_t UnixTime) {

    RTCC_STATUS stat;

    struct tm* time = ConvertUnixTimeToRealTime(&UnixTime);
    stat = RTCCSetTimeDateDecimal(time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, time->tm_mon, time->tm_year, 0x01, 0x01);
    
    return stat;
}

RTCC_STATUS RTCCGetTimeDateBCD(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // packed data structure
    uint8_t TimeDateRegisters[7];

    // read the time, date, and settings registers from the RTCC
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 7, TimeDateRegisters);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // unpack the registers into the calling function tm struct
    TimeDate->tm_sec = TimeDateRegisters[0] & 0x7F;
    TimeDate->tm_min = TimeDateRegisters[1];
    TimeDate->tm_hour = TimeDateRegisters[2] & 0x3F;
    TimeDate->tm_mday = TimeDateRegisters[4] & 0x3F;
    TimeDate->tm_mon = TimeDateRegisters[5] & 0x1F;
    TimeDate->tm_year = TimeDateRegisters[6];

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetTimeDateDecimal(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // BCD time date struct
    struct tm TimeDateBCD;

    // Get the time and date in BCD
    stat = RTCCGetTimeDateBCD((struct tm *)&TimeDateBCD);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // convert from BCD to decimal (HH:MM:SS MM/DD/YYYY)
    TimeDate->tm_sec = (TimeDateBCD.tm_sec & 0x0F) + 10*(TimeDateBCD.tm_sec>>4);
    TimeDate->tm_min = (TimeDateBCD.tm_min & 0x0F) + 10*(TimeDateBCD.tm_min>>4);
    TimeDate->tm_hour = (TimeDateBCD.tm_hour & 0x0F) + 10*(TimeDateBCD.tm_hour>>4);
    TimeDate->tm_mday = (TimeDateBCD.tm_mday & 0x0F) + 10*(TimeDateBCD.tm_mday>>4);
    TimeDate->tm_mon = (TimeDateBCD.tm_mon & 0x0F) + 10*(TimeDateBCD.tm_mon>>4);
    TimeDate->tm_year = ((TimeDateBCD.tm_year & 0x0F) + 10*(TimeDateBCD.tm_year>>4)) + 2000;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetPowerDownTimestampBCD(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // packed data structure
    char TimestampRegisters[4];

    // read the time, date, and settings registers from the RTCC
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, RTCC_POWERDOWN_STARTADDR, 4, TimestampRegisters);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // unpack the registers into the calling function tm struct
    TimeDate->tm_sec = 0x0;
    TimeDate->tm_min = TimestampRegisters[0] & 0x7F;
    TimeDate->tm_hour = TimestampRegisters[1] & 0x3F;
    TimeDate->tm_mday = TimestampRegisters[2] & 0x3F;
    TimeDate->tm_mon = TimestampRegisters[3] & 0x1F;
    TimeDate->tm_year = 0x0;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetPowerDownTimestampDecimal(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // BCD time date struct
    struct tm TimestampBCD;

    // Get the timestamp in BCD
    stat = RTCCGetPowerDownTimestampBCD((struct tm *)&TimestampBCD);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // convert from BCD to decimal (HH:MM:SS MM/DD/YYYY)
    TimeDate->tm_sec = (TimestampBCD.tm_sec & 0x0F) + 10*(TimestampBCD.tm_sec>>4);
    TimeDate->tm_min = (TimestampBCD.tm_min & 0x0F) + 10*(TimestampBCD.tm_min>>4);
    TimeDate->tm_hour = (TimestampBCD.tm_hour & 0x0F) + 10*(TimestampBCD.tm_hour>>4);
    TimeDate->tm_mday = (TimestampBCD.tm_mday & 0x0F) + 10*(TimestampBCD.tm_mday>>4);
    TimeDate->tm_mon = (TimestampBCD.tm_mon & 0x0F) + 10*(TimestampBCD.tm_mon>>4);
    TimeDate->tm_year = ((TimestampBCD.tm_year & 0x0F) + 10*(TimestampBCD.tm_year>>4)) + 2000;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetPowerUpTimestampBCD(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // packed data structure
    char TimestampRegisters[4];

    // read the time, date, and settings registers from the RTCC
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, RTCC_POWERUP_STARTADDR, 4, TimestampRegisters);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // unpack the registers into the calling function tm struct
    TimeDate->tm_sec = 0x0;
    TimeDate->tm_min = TimestampRegisters[0] & 0x7F;
    TimeDate->tm_hour = TimestampRegisters[1] & 0x3F;
    TimeDate->tm_mday = TimestampRegisters[2] & 0x3F;
    TimeDate->tm_mon = TimestampRegisters[3] & 0x1F;
    TimeDate->tm_year = 0x0;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCGetPowerUpTimestampDecimal(struct tm * TimeDate) {

    // return code
    RTCC_STATUS stat;
    // BCD time date struct
    struct tm TimestampBCD;

    // Get the timestamp in BCD
    stat = RTCCGetPowerUpTimestampBCD((struct tm *)&TimestampBCD);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // convert from BCD to decimal (HH:MM:SS MM/DD/YYYY)
    TimeDate->tm_sec = (TimestampBCD.tm_sec & 0x0F) + 10*(TimestampBCD.tm_sec>>4);
    TimeDate->tm_min = (TimestampBCD.tm_min & 0x0F) + 10*(TimestampBCD.tm_min>>4);
    TimeDate->tm_hour = (TimestampBCD.tm_hour & 0x0F) + 10*(TimestampBCD.tm_hour>>4);
    TimeDate->tm_mday = (TimestampBCD.tm_mday & 0x0F) + 10*(TimestampBCD.tm_mday>>4);
    TimeDate->tm_mon = (TimestampBCD.tm_mon & 0x0F) + 10*(TimestampBCD.tm_mon>>4);
    TimeDate->tm_year = ((TimestampBCD.tm_year & 0x0F) + 10*(TimestampBCD.tm_year>>4)) + 2000;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCClearTimestamps(void) {

    // return code
    RTCC_STATUS stat;
    // day register value
    uint8_t DayRegister;

    // read the day register
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, 0x03, 1, &DayRegister);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // flip the Vbat bit
    DayRegister &= (DayRegister & 0xEF);

    // write the day register
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, 0x03, 1, &DayRegister);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCSetAlarmBCD(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t alarmType) {

    RTCC_STATUS stat;
    uint8_t AlarmRegisters[6];
    uint8_t EnableAlarm0 = 0x10;

    // set the alarm time
    AlarmRegisters[0] = second;
    AlarmRegisters[1] = minute;
    AlarmRegisters[2] = hour;
    AlarmRegisters[3] = ((alarmType & 0x07) << 4) + (0x80);
    AlarmRegisters[4] = date;
    AlarmRegisters[5] = month;
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_ALARM0_STARTADDR, 6, AlarmRegisters);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // turn on the alarm
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_CONTROLREGISTER_STARTADDR, 1, &EnableAlarm0);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCSetAlarmDecimal(uint8_t second, uint8_t minute, uint8_t hour, uint8_t date, uint8_t month, uint8_t alarmType) {

    RTCC_STATUS stat;
    uint8_t AlarmRegisters[6];
    uint8_t EnableAlarm0 = 0x10;

    // convert the decimal values to BCD
    AlarmRegisters[0] = (second % 10) + ((second/10)<<4);
    AlarmRegisters[1] = (minute % 10) + ((minute/10)<<4);
    AlarmRegisters[2] = (hour % 10) + ((hour/10)<<4);
    AlarmRegisters[3] = ((alarmType & 0x07) << 4) + (0x80);
    AlarmRegisters[4] = (date % 10) + ((date/10)<<4);
    AlarmRegisters[5] = (month % 10) + ((month/10)<<4);

    // set the alarm
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_ALARM0_STARTADDR, 6, AlarmRegisters);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    // turn on the alarm
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_CONTROLREGISTER_STARTADDR, 1, &EnableAlarm0);
    if(stat != RTCC_NO_ERROR) goto fcn_exit;

    fcn_exit:

    return stat;

}

RTCC_STATUS RTCCSetCalibrationRegister(INT8 reg_val) {

    RTCC_STATUS stat;
    uint8_t reg_set[1];

    // set the lower 7 bits to the absolute value
    reg_set[0] = ( ( (reg_val < 0) ? (0 - reg_val) : reg_val ) & 0x7F );      
    // set MSB sign bit
    reg_set[0] += (reg_val < 0) << 7;

    // write to the RTCC SRAM
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, 0x08, 1, reg_set);

    return stat;

}

time_t ConvertRealTimeToUnixTime(struct tm * RealTime) {

    struct tm UnixTime;

    // copy the time into Unix time
    UnixTime = *RealTime;

    // modify the year and month index
    UnixTime.tm_year -= 1900;
    UnixTime.tm_mon--;

    // get the Unix time
    return mktime((struct tm *)&UnixTime);

}

struct tm * ConvertUnixTimeToRealTime(time_t * UnixTime) {

    struct tm * RealTime;

    RealTime = gmtime(UnixTime);
    
    if(RealTime == NULL) return NULL;
    
    RealTime->tm_year += 1900;
    RealTime->tm_mon++;

    return RealTime;

}

time_t RTCCGetTimeDateUnix(void) {

    RTCC_STATUS stat;
    struct tm TimeDate;

    stat = RTCCGetTimeDateDecimal((struct tm *)&TimeDate);

    if(stat == RTCC_NO_ERROR)
    {
        return(ConvertRealTimeToUnixTime((struct tm *)&TimeDate));
    }
    else
    {
        return 0xFFFFFFFF;
    }

}

// crystal
RTCC_STATUS RTCCGetCrystalState(bool *crystal_state)
{
    // return code
    RTCC_STATUS stat;
    uint8_t reg = 0;

    // get the bytes
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 1, &reg);
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }
    *crystal_state = (bool)(reg & RTCC_ST_MASK);

    fcn_exit:

    return stat;
}

RTCC_STATUS RTCCSetCrystalState(bool crystal_state)
{
    // return code
    RTCC_STATUS stat;
    uint8_t reg = 0;

    // get the bytes
    stat = RTCC_ReadBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 1, &reg);
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }

    // set the ST bit
    reg |= ((uint8_t)crystal_state << 7);

    delay_ms(5);

    // set the bytes
    stat = RTCC_WriteBytes(RTCC_MEMTYPE_SRAM, RTCC_TIMEDATE_STARTADDR, 1, &reg);
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }

    fcn_exit:

    return stat;
}

/*PRIVIATE FUNCTIONS*/

RTCC_STATUS I2C_Reset() {

    // return code
    RTCC_STATUS stat;
    // loop counter
    uint8_t i;

    /*
     * Disable the I2C peripheral
     * We want manual control of the bus in case the slave is hanging it,
     * which will cause the I2C peripheral to fail to initiate the transactions
     * required to recover the bus.
     */
    stat = I2C_Disable();
    if(stat != RTCC_NO_ERROR)
    {
        goto fcn_exit;
    }

    /*
     * Tri-state the I2C bus pins so we can read bus condition.
     */
    BUS_RELEASE();

    /*
     *  SLAVE IS HANGING WHILE THE BUS IS FLOATING
     *
     *  If the bus is floating, then we can drive a start condition on the
     *  bus to signal the slave to reset.
     *
     *  This would typically happen if the slave is in receive mode, waiting
     *  for bits from the master. It can also happen if the slave is attempting
     *  to ACK something that the master previously transmitted, in which case
     *  the slave is waiting for a CLOCK pulse.
     */
    if(SDA_STATE && SCL_STATE)
    {
        // pull SDA low
        SDA_LOW();
        delay_ms(1);        
        // pull SCL low
        SCL_LOW();
        delay_ms(1);   
    }
    /*
     *  SLAVE IS HANGING WHILE THE BUS IS BEING DRIVEN
     *
     *  If the bus is being driven low, then we cannot signal a start condition.
     *  In this case, we will need to send clock pulses out until the slave
     *  releases the bus. The slave will release the bus when either a '1'
     *  bit is ready to transmit, or when it is finished transmitting bits
     *  and wants the master to ACK. As soon as the slave releases the bus, we
     *  will signal a start condition to cancel the transaction WITHOUT
     *  allowing the slave to commit that transaction to memory.
     */
    else
    {
        /*PULSE CLOCK UP TO NINE TIMES UNTIL SDA FLOATS*/

        // clock up to nine bits
        for(i = 0; i < 9; i++)
        {
            // pull SCL low
            SCL_LOW();
            
            delay_ms(1);   
            // break if the bus has been released
            
            if(SDA_STATE == 1) break;
            // float SCL high
            SCL_RELEASE();
            
            delay_ms(1);   
        }

         delay_ms(1);   

        /*SEND START CONDITION*/

        // release bus
        BUS_RELEASE();
        
        delay_ms(1);   
        // pull SDA low
        SDA_LOW();
        
        delay_ms(1);   
        // pull SCL low
        SCL_LOW();
        
        delay_ms(1);   

    }

    /*
     *  STOP CONDITION
     *  Regardless of the path (start or clocks + start) needed to get the slave
     *  to reset, we are now in a known state, having just started a new
     *  transaction. Now we send the stop condition to end the transaction
     *  and place the slave in an idle state.
     */
    // float SCL high
    SCL_RELEASE();

    delay_ms(1);   

    SDA_RELEASE();

    delay_ms(1);   

    // release the I2C bus so we can sample its condition
    BUS_RELEASE();

    // if bus is floating, we have had success in releasing slave control
    if(SCL_STATE && SDA_STATE)
    {
        // return success
        stat = RTCC_NO_ERROR;
    }
    // if one of the bus pins is still being driven by the slave, we failed
    else
    {
        // return reset error
        stat = RTCC_I2C_RESET_FAILED;
    }

    fcn_exit:

    return stat;

}

RTCC_STATUS I2C_Start() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;
    uint32_t status = 0x00000000;

    // start I2C start sequence
    res = I2CStart(RTCC_I2C_MODULE);
    if(res != I2C_SUCCESS)
    {
        return RTCC_I2C_START_COLLISION;
    }
    
    // block until start sequence is complete
    dms =  millis();
    //while(I2C1CONbits.SEN);
    uint32_t s = status & I2C_START;
    while((status & I2C_START) == 0)
    {
        status = I2CGetStatus(RTCC_I2C_MODULE);
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_SENDSTART_TIMEOUT;
        }
    }

    return RTCC_NO_ERROR;
    
}

RTCC_STATUS I2C_Restart() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;
    uint32_t status = 0x00000000;

    // start I2C restart sequence
    res = I2CRepeatStart(RTCC_I2C_MODULE);
    if(res != I2C_SUCCESS)
    {
       return RTCC_I2C_RESTART_FAILED;
    }
    //I2C5CONSET = (1 << 1);

    // block until restart sequence is complete
    dms = millis();
    //while(I2C5CONbits.RSEN)
    uint32_t s = status & I2C_START;
    while((status & I2C_START) == 0)
    {
        status = I2CGetStatus(RTCC_I2C_MODULE);
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_SENDRESTART_TIMEOUT;
        }
    }

    return RTCC_NO_ERROR;

}

RTCC_STATUS I2C_Stop() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;

    // start I2C stop sequence
    I2CStop(RTCC_I2C_MODULE);
    //I2C5CONSET = (1 << 2);

    // block until stop sequence is complete
    dms = millis();
    while(I2CBusIsIdle(RTCC_I2C_MODULE) == FALSE)
    //while(I2C5CONbits.PEN)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_SENDSTOP_TIMEOUT;
        }
    }

    return RTCC_NO_ERROR;
}

RTCC_STATUS I2C_Receive() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;

    // start I2C receive sequence
    if(I2CReceiverEnable(RTCC_I2C_MODULE, TRUE) != I2C_SUCCESS)
    {
        return RTCC_I2C_RECEIVE_FAILED;
    }
    //I2C5CONSET = (1 << 3);

    // block until receive sequence is complete
    dms = millis();
    //while(I2C5CONbits.RCEN)
    while(I2CReceivedDataIsAvailable(RTCC_I2C_MODULE) == FALSE)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_RECEIVEBYTE_TIMEOUT;
        }
    }

    return RTCC_NO_ERROR;

}

RTCC_STATUS I2C_Acknowledge_ACK() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;

    // set acknowledge type ACK
    //I2C5CONCLR = (1 << 5);

    // start acknowledge sequence
    I2CAcknowledgeByte(RTCC_I2C_MODULE, TRUE);
    //I2C5CONSET = (1 << 4);

    // block until acknowledge sequence is complete
    dms = millis();
    //while(I2C5CONbits.ACKEN)
    while(I2CAcknowledgeHasCompleted(RTCC_I2C_MODULE) == FALSE)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_ACK_TIMEOUT;
        }
    }
    
    return RTCC_NO_ERROR;

}

RTCC_STATUS I2C_Acknowledge_NACK() {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;

    // set acknowledge type NACK
    //I2C5CONSET = (1 << 5);

    // start acknowledge sequence
    I2CAcknowledgeByte(RTCC_I2C_MODULE, FALSE);
    //I2C5CONSET = (1 << 4);

    // block until acknowledge sequence is complete
    dms = millis();
    //while(I2C5CONbits.ACKEN)
    while(I2CAcknowledgeHasCompleted(RTCC_I2C_MODULE) == FALSE)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_NACK_TIMEOUT;
        }
    }

    return RTCC_NO_ERROR;

}

RTCC_STATUS I2C_Transmit(uint8_t data) {

    I2C_RESULT res = I2C_SUCCESS;
    uint32_t dms;

    // block until transmit buffer is available
    dms = millis();
    //while(I2C5STATbits.TBF)
    while(I2CTransmitterIsReady(RTCC_I2C_MODULE) == FALSE)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_SENDBYTE_TIMEOUT;
        }
    }

    // load transmit buffer and start transmit sequence
    res = I2CSendByte(RTCC_I2C_MODULE, data);
    if(res != I2C_SUCCESS)
    {
        return RTCC_I2C_SEND_FAILED;
    }
    //I2C5TRN = data;

    // block until transmit sequence is complete
    dms = millis();
    //while(I2C5STATbits.TRSTAT)
    while(I2CTransmissionHasCompleted(RTCC_I2C_MODULE) == FALSE)
    {
        if(millis() - dms > I2C_MAX_MILLISECONDS_EVENT)
        {
            return RTCC_I2C_SENDBYTE_TIMEOUT;
        }
    }

    // confirm positive ACK received, indicating successful byte transfer
    //if(I2C5STATbits.ACKSTAT)
    if(I2CByteWasAcknowledged(RTCC_I2C_MODULE) == FALSE)
    {
        return RTCC_I2C_TRANSMIT_NOACK;
    }

    return RTCC_NO_ERROR;

}
#endif
