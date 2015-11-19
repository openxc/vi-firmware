/*******************************************************************************
 Basic SPI Driver

  Company:
    Microchip Technology Inc.

  File Name:
    drv_spi_16bit.c

  Summary:
    The is the SPI driver file for PIC24 and dsPIC.
    Use this implementation for SPI peripheral version without Audio Codec support.

  Description:
    The is the SPI driver file for PIC24 and dsPIC.
    Use this implementation for SPI peripheral version without Audio Codec support.

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/
// DOM-IGNORE-END

#include <xc.h>
#include "drv_spi.h"
#include "system_config.h"
#include "system.h"
#include <stdint.h>

// error checks
#if !defined(DRV_SPI_CONFIG_CHANNEL_1_ENABLE) && !defined(DRV_SPI_CONFIG_CHANNEL_2_ENABLE) && !defined(DRV_SPI_CONFIG_CHANNEL_3_ENABLE) && !defined(DRV_SPI_CONFIG_CHANNEL_4_ENABLE)
    #warning "No SPI Channel defined! Please define in system_config.h or system.h"
#endif

#if defined(__PIC24FJ128GA010__) || defined(__PIC24FJ96GA010__) || defined(__PIC24FJ64GA010__) || defined(__PIC24FJ128GA008__) || defined(__PIC24FJ96GA008__) || defined(__PIC24FJ64GA008__) || defined(__PIC24FJ128GA006__) || defined(__PIC24FJ96GA006__) || defined(__PIC24FJ64GA006__)
  #ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
  #define DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
  #pragma message "DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE is automatically enabled because SPI FIFO mode is not supported on this device."
  #endif
#endif

/**
  DRV_SPIx_CONFIG_DUMMY_DATA

  @Summary
    Dummy data to be sent.

  @Description
    Dummy data to be sent, when no input buffer is specified in the buffer APIs.
 */
#ifndef DRV_SPI1_CONFIG_DUMMY_DATA
    #define DRV_SPI1_CONFIG_DUMMY_DATA 0xFF
#endif
#ifndef DRV_SPI2_CONFIG_DUMMY_DATA
    #define DRV_SPI2_CONFIG_DUMMY_DATA 0xFF
#endif
#ifndef DRV_SPI3_CONFIG_DUMMY_DATA
    #define DRV_SPI3_CONFIG_DUMMY_DATA 0xFF
#endif
#ifndef DRV_SPI4_CONFIG_DUMMY_DATA
    #define DRV_SPI4_CONFIG_DUMMY_DATA 0xFF
#endif

/* SPI SFR definitions. i represents the SPI
   channel number.
   valid i values are: 1, 2, 3, 4
*/
#define DRV_SPI_STAT(i)      SPI##i##STAT
#define DRV_SPI_STATbits(i)  SPI##i##STATbits
#define DRV_SPI_CON(i)       SPI##i##CON
#define DRV_SPI_CONbits(i)   SPI##i##CONbits
#define DRV_SPI_CON2(i)      SPI##i##CON2
#define DRV_SPI_CON2bits(i)  SPI##i##CON2bits
#define DRV_SPI_BUF(i)       SPI##i##BUF
#define DRV_SPI_BUFbits(i)   SPI##i##BUFbits
#define DRV_SPI_BRG(i)       SPI##i##BRG

static int spiMutex[4] = { 0, 0, 0, 0 };

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
static uint8_t spi1DummyData = DRV_SPI1_CONFIG_DUMMY_DATA;
#endif

#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
static uint8_t spi2DummyData = DRV_SPI2_CONFIG_DUMMY_DATA;
#endif

#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
static uint8_t spi3DummyData = DRV_SPI3_CONFIG_DUMMY_DATA;
#endif

#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
static uint8_t spi4DummyData = DRV_SPI4_CONFIG_DUMMY_DATA;
#endif

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte1 (void);
#endif
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte2 (void);
#endif
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte3 (void);
#endif
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte4 (void);
#endif

/*****************************************************************************
 * void DRV_SPI_Initialize(const unsigned int channel, DRV_SPI_INIT_DATA *pData)
 *****************************************************************************/
void DRV_SPI_Initialize(DRV_SPI_INIT_DATA *pData)
{

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE        
    if (pData->channel == 1)
    {

        DRV_SPI_STAT(1) = 0;

        DRV_SPI_CON(1) = 0;
        DRV_SPI_CONbits(1).MSTEN = SPI_MST_MODE_ENABLE;
        DRV_SPI_CONbits(1).MODE16 = (pData->mode)& 0x01;
        DRV_SPI_CONbits(1).CKE = pData->cke;
        switch (pData->spibus_mode)
        {
            case SPI_BUS_MODE_0:
                //smp = 0, ckp = 0
                DRV_SPI_CONbits(1).CKP = 0;
                DRV_SPI_CONbits(1).SMP = 0;
                break;
            case SPI_BUS_MODE_1:
                //smp = 1, ckp = 0
                DRV_SPI_CONbits(1).CKP = 0;
                DRV_SPI_CONbits(1).SMP = 1;
                break;
            case SPI_BUS_MODE_2:
                //smp = 0, ckp = 1
                DRV_SPI_CONbits(1).CKP = 1;
                DRV_SPI_CONbits(1).SMP = 0;
                break;
            case SPI_BUS_MODE_3:
                //smp = 1, ckp = 1
                DRV_SPI_CONbits(1).CKP = 1;
                DRV_SPI_CONbits(1).SMP = 1;
                break;
            default:
                // should not happen
                break;
        }   

    #ifdef __PIC32MX
        DRV_SPI_CONbits(1).MODE32 = ((pData->mode) >> 1)& 0x01;
        DRV_SPI_BRG(1) = pData->baudRate;
        DRV_SPI_CONbits(1).ON = SPI_MODULE_ENABLE;
    #else
        DRV_SPI_CON2(1) = 0;
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_CON2bits(1).SPIBEN = 1;
#endif
        DRV_SPI_CONbits(1).PPRE= pData->primaryPrescale;
        DRV_SPI_CONbits(1).SPRE= pData->secondaryPrescale;
        DRV_SPI_STATbits(1).SPIEN = SPI_MODULE_ENABLE;
    #endif
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (pData->channel == 2)
    {

        DRV_SPI_STAT(2) = 0;

        DRV_SPI_CON(2) = 0;
        DRV_SPI_CONbits(2).MSTEN = SPI_MST_MODE_ENABLE;
        DRV_SPI_CONbits(2).MODE16 = (pData->mode)& 0x01;
        DRV_SPI_CONbits(2).CKE = pData->cke;
        switch (pData->spibus_mode)
        {
            case SPI_BUS_MODE_0:
                //smp = 0, ckp = 0
                DRV_SPI_CONbits(2).CKP = 0;
                DRV_SPI_CONbits(2).SMP = 0;
                break;
            case SPI_BUS_MODE_1:
                //smp = 1, ckp = 0
                DRV_SPI_CONbits(2).CKP = 0;
                DRV_SPI_CONbits(2).SMP = 1;
                break;
            case SPI_BUS_MODE_2:
                //smp = 0, ckp = 1
                DRV_SPI_CONbits(2).CKP = 1;
                DRV_SPI_CONbits(2).SMP = 0;
                break;
            case SPI_BUS_MODE_3:
                //smp = 1, ckp = 1
                DRV_SPI_CONbits(2).CKP = 1;
                DRV_SPI_CONbits(2).SMP = 1;
                break;
            default:
                // should not happen
                break;
        }           

    #ifdef __PIC32MX
        DRV_SPI_CONbits(2).MODE32 = ((pData->mode) >> 1)& 0x01;
        DRV_SPI_BRG(2) = pData->baudRate;
        DRV_SPI_CONbits(2).ON = SPI_MODULE_ENABLE;
    #else
        DRV_SPI_CON2(2) = 0;
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_CON2bits(2).SPIBEN = 1;
#endif
        DRV_SPI_CONbits(2).PPRE= pData->primaryPrescale;
        DRV_SPI_CONbits(2).SPRE= pData->secondaryPrescale;
        DRV_SPI_STATbits(2).SPIEN = SPI_MODULE_ENABLE;
    #endif
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (pData->channel == 3)
    {

        DRV_SPI_STAT(3) = 0;

        DRV_SPI_CON(3) = 0;
        DRV_SPI_CONbits(3).MSTEN = SPI_MST_MODE_ENABLE;
        DRV_SPI_CONbits(3).MODE16 = (pData->mode)& 0x01;
        DRV_SPI_CONbits(3).CKE = pData->cke;
        switch (pData->spibus_mode)
        {
            case SPI_BUS_MODE_0:
                //smp = 0, ckp = 0
                DRV_SPI_CONbits(3).CKP = 0;
                DRV_SPI_CONbits(3).SMP = 0;
                break;
            case SPI_BUS_MODE_1:
                //smp = 1, ckp = 0
                DRV_SPI_CONbits(3).CKP = 0;
                DRV_SPI_CONbits(3).SMP = 1;
                break;
            case SPI_BUS_MODE_2:
                //smp = 0, ckp = 1
                DRV_SPI_CONbits(3).CKP = 1;
                DRV_SPI_CONbits(3).SMP = 0;
                break;
            case SPI_BUS_MODE_3:
                //smp = 1, ckp = 1
                DRV_SPI_CONbits(3).CKP = 1;
                DRV_SPI_CONbits(3).SMP = 1;
                break;
            default:
                // should not happen
                break;
        }           
        
    #ifdef __PIC32MX
        DRV_SPI_CONbits(3).MODE32 = ((pData->mode) >> 1)& 0x01;
        DRV_SPI_BRG(3) = pData->baudRate;
        DRV_SPI_CONbits(3).ON = SPI_MODULE_ENABLE;
    #else
        DRV_SPI_CON2(3) = 0;
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_CON2bits(3).SPIBEN = 1;
#endif
        DRV_SPI_CONbits(3).PPRE= pData->primaryPrescale;
        DRV_SPI_CONbits(3).SPRE= pData->secondaryPrescale;
        DRV_SPI_STATbits(3).SPIEN = SPI_MODULE_ENABLE;
    #endif
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (pData->channel == 4)
    {

        DRV_SPI_STAT(4) = 0;

        DRV_SPI_CON(4) = 0;
        DRV_SPI_CONbits(4).MSTEN = SPI_MST_MODE_ENABLE;
        DRV_SPI_CONbits(4).MODE16 = (pData->mode)& 0x01;
        DRV_SPI_CONbits(4).CKE = pData->cke;
        switch (pData->spibus_mode)
        {
            case SPI_BUS_MODE_0:
                //smp = 0, ckp = 0
                DRV_SPI_CONbits(4).CKP = 0;
                DRV_SPI_CONbits(4).SMP = 0;
                break;
            case SPI_BUS_MODE_1:
                //smp = 1, ckp = 0
                DRV_SPI_CONbits(4).CKP = 0;
                DRV_SPI_CONbits(4).SMP = 1;
                break;
            case SPI_BUS_MODE_2:
                //smp = 0, ckp = 1
                DRV_SPI_CONbits(4).CKP = 1;
                DRV_SPI_CONbits(4).SMP = 0;
                break;
            case SPI_BUS_MODE_3:
                //smp = 1, ckp = 1
                DRV_SPI_CONbits(4).CKP = 1;
                DRV_SPI_CONbits(4).SMP = 1;
                break;
            default:
                // should not happen
                break;
        }
        
    #ifdef __PIC32MX
        DRV_SPI_CONbits(4).MODE32 = ((pData->mode) >> 1)& 0x01;
        DRV_SPI_BRG(4) = pData->baudRate;
        DRV_SPI_CONbits(4).ON = SPI_MODULE_ENABLE;
    #else
        DRV_SPI_CON2(4) = 0;
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_CON2bits(4).SPIBEN = 1;
#endif
        DRV_SPI_CONbits(4).PPRE= pData->primaryPrescale;
        DRV_SPI_CONbits(4).SPRE= pData->secondaryPrescale;
        DRV_SPI_STATbits(4).SPIEN = SPI_MODULE_ENABLE;
    #endif
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE

}

void DRV_SPI_Deinitialize (uint8_t channel)
{
	
#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
        DRV_SPI_STATbits(1).SPIEN = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
#warning "Ignored! SPI disable"
        DRV_SPI_CONbits(3).ON = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
        DRV_SPI_CONbits(3).ON = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
        DRV_SPI_STATbits(4).SPIEN = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE

}

/*****************************************************************************
void SPI_DummyDataSet(
                        uint8_t channel,
                        uint8_t dummyData)
 *****************************************************************************/
void SPI_DummyDataSet(
                        uint8_t channel,
                        uint8_t dummyData)
{
	/*
#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
        spi1DummyData = dummyData;
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
        spi2DummyData = dummyData;
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
        spi3DummyData = dummyData;
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
        spi4DummyData = dummyData;
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
*/

}


#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
    #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte1 (void)
    {
       while (DRV_SPI_STATbits(1).SRMPT);
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte2 (void)
    {
        while (SPI2STAT & 0x80);
		//while ((SPI2STAT & (1<<1)) == (1<<1)) ; //check SRMT bit
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte3 (void)
    {
        while (DRV_SPI_STATbits(3).SRXMPT);
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte4 (void)
    {
        while (DRV_SPI_STATbits(4).SRXMPT);
    }
    #endif
#else
    #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte1 (void)
    {
        while (!DRV_SPI_STATbits(1).SPIRBF);
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte2 (void)
    {
        while (!DRV_SPI_STATbits(2).SPIRBF);
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte3 (void)
    {
        while (!DRV_SPI_STATbits(3).SPIRBF);
    }
    #endif

    #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    static inline __attribute__((__always_inline__)) void DRV_SPI_WaitForDataByte4 (void)
    {
        while (!DRV_SPI_STATbits(4).SPIRBF);
    }
    #endif
#endif

/*****************************************************************************
 * void SPIPut(unsigned int channel, unsigned char data)
 *****************************************************************************/
void DRV_SPI_Put(uint8_t channel, uint8_t data)
{

    uint8_t clear;
#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
    #ifdef __PIC32MX
        // Wait for free buffer
        while(!DRV_SPI_STATbits(1).SPITBE);
    #else
        // Wait for free buffer
        while(DRV_SPI_STATbits(1).SPITBF);
    #endif
        DRV_SPI_BUF(1) = data;

        // Wait for data uint8_t
        DRV_SPI_WaitForDataByte1();
        clear = DRV_SPI_BUF(1);
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
    #ifdef __PIC32MX
        // Wait for free buffer
        while(!DRV_SPI_STATbits(2).SPITBE);
    #else
        // Wait for free buffer
        while(DRV_SPI_STATbits(2).SPITBF);
    #endif
        DRV_SPI_BUF(2) = data;
        // Wait for data uint8_t
        DRV_SPI_WaitForDataByte2();
        clear = DRV_SPI_BUF(2);
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
    #ifdef __PIC32MX
        // Wait for free buffer
        while(!DRV_SPI_STATbits(3).SPITBE);
    #else
        // Wait for free buffer
        while(DRV_SPI_STATbits(3).SPITBF);
    #endif
        DRV_SPI_BUF(3) = data;
        // Wait for data uint8_t
        DRV_SPI_WaitForDataByte3();
        clear = DRV_SPI_BUF(3);
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
    #ifdef __PIC32MX
        // Wait for free buffer
        while(!DRV_SPI_STATbits(4).SPITBE);
    #else
        // Wait for free buffer
        while(DRV_SPI_STATbits(4).SPITBF);
    #endif
        DRV_SPI_BUF(4) = data;
        // Wait for data uint8_t
        DRV_SPI_WaitForDataByte4();
        clear = DRV_SPI_BUF(4);
        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
}

void DRV_SPI_PutBuffer(uint8_t channel, uint8_t * data, uint16_t count)
{

    uint8_t dataByte;
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
    uint16_t originalLength = count;
#endif

    if (count == 0)
    {
        return;
    }

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(1) = *data++;
            count--;
        }
        DRV_SPI_BUF(1) = *data++;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(1) = *data++;
            DRV_SPI_WaitForDataByte1();
            dataByte = DRV_SPI_BUF(1);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte1();
        dataByte = DRV_SPI_BUF(1);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte1();
            dataByte = DRV_SPI_BUF(1);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(2) = *data++;
            count--;
        }
        DRV_SPI_BUF(2) = *data++;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(2) = *data++;
            DRV_SPI_WaitForDataByte2();
            dataByte = DRV_SPI_BUF(2);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte2();
        dataByte = DRV_SPI_BUF(2);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte2();
            dataByte = DRV_SPI_BUF(2);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(3) = *data++;
            count--;
        }
        DRV_SPI_BUF(3) = *data++;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(3) = *data++;
            DRV_SPI_WaitForDataByte3();
            dataByte = DRV_SPI_BUF(3);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte3();
        dataByte = DRV_SPI_BUF(3);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte3();
            dataByte = DRV_SPI_BUF(3);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(4) = *data++;
            count--;
        }
        DRV_SPI_BUF(4) = *data++;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(4) = *data++;
            DRV_SPI_WaitForDataByte4();
            dataByte = DRV_SPI_BUF(4);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte4();
        dataByte = DRV_SPI_BUF(4);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte4();
            dataByte = DRV_SPI_BUF(4);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
}

/*****************************************************************************
 * uint8_t SPIGet (unsigned int channel)
 *****************************************************************************/
uint8_t DRV_SPI_Get (uint8_t channel)
{

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
        DRV_SPI_BUF(1) = spi1DummyData;
        DRV_SPI_WaitForDataByte1();
        return DRV_SPI_BUF(1);
    }
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
        DRV_SPI_BUF(2) = spi2DummyData;
        DRV_SPI_WaitForDataByte2();
        return DRV_SPI_BUF(2);
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
        DRV_SPI_BUF(3) = spi3DummyData;
        DRV_SPI_WaitForDataByte3();
        return DRV_SPI_BUF(3);
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
        DRV_SPI_BUF(4) = spi4DummyData;
        DRV_SPI_WaitForDataByte4();
        return DRV_SPI_BUF(4);
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    return 0x00;
}

void DRV_SPI_GetBuffer(uint8_t channel, uint8_t * data, uint16_t count)
{

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
    uint16_t originalLength = count;
#endif
    
    if (count == 0)
    {
        return;
    }

#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(1) = spi1DummyData;
            count--;
        }
        DRV_SPI_BUF(1) = spi1DummyData;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(1) = spi1DummyData;
            DRV_SPI_WaitForDataByte1();
            *data++ = DRV_SPI_BUF(1);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte1();
        *data++ = DRV_SPI_BUF(1);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte1();
            *data++ = DRV_SPI_BUF(1);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
        #ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(2) = spi2DummyData;
            count--;
        }
        DRV_SPI_BUF(2) = spi2DummyData;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(2) = spi2DummyData;
            DRV_SPI_WaitForDataByte2();
            *data++ = DRV_SPI_BUF(2);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte2();
        *data++ = DRV_SPI_BUF(2);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte2();
            *data++ = DRV_SPI_BUF(2);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
        #ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(3) = spi3DummyData;
            count--;
        }
        DRV_SPI_BUF(3) = spi3DummyData;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(3) = spi3DummyData;
            DRV_SPI_WaitForDataByte3();
            *data++ = DRV_SPI_BUF(3);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte3();
        *data++ = DRV_SPI_BUF(3);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte3();
            *data++ = DRV_SPI_BUF(3);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
        #ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        if (originalLength >= 2)
        {
            DRV_SPI_BUF(4) = spi4DummyData;
            count--;
        }
        DRV_SPI_BUF(4) = spi4DummyData;
        count--;
#endif
        while (count--)
        {
            DRV_SPI_BUF(4) = spi4DummyData;
            DRV_SPI_WaitForDataByte4();
            *data++ = DRV_SPI_BUF(4);
        }

#ifndef DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
        DRV_SPI_WaitForDataByte4();
        *data++ = DRV_SPI_BUF(4);
        if (originalLength >= 2)
        {
            DRV_SPI_WaitForDataByte4();
            *data++ = DRV_SPI_BUF(4);
        }
#endif

        return;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
}

/*****************************************************************************
 * int SPILock(unsigned int channel)
 *****************************************************************************/
int DRV_SPI_Lock(uint8_t channel)
{
#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
        if(!spiMutex[0])
        {
            spiMutex[0] = 1;
            return 1;
        }

        return 0;
    }
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
        if(!spiMutex[1])
        {
            spiMutex[1] = 1;
            return 1;
        }

        return 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
        if(!spiMutex[2])
        {
            spiMutex[2] = 1;
            return 1;
        }

        return 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
        if(!spiMutex[3])
        {
            spiMutex[3] = 1;
            return 1;
        }

        return 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    return -1;
}

/*****************************************************************************
 * int SPIUnLock(unsigned int channel)
 *****************************************************************************/
void DRV_SPI_Unlock(uint8_t channel)
{
#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
    if (channel == 1)
    {
        spiMutex[0] = 0;
    }
#endif //#ifdef DRV_SPI_CONFIG_CHANNEL_1_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
    if (channel == 2)
    {
        spiMutex[1] = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_2_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
    if (channel == 3)
    {
        spiMutex[2] = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_3_ENABLE
#ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
    if (channel == 4)
    {
        spiMutex[3] = 0;
    }
#endif // #ifdef DRV_SPI_CONFIG_CHANNEL_4_ENABLE
}
