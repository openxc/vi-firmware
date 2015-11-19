/******************************************************************************
*
*                        Microchip SPI Driver Configuration
*
******************************************************************************
* FileName:           drv_spi_config_default.h
* Processor:          PIC18/PIC24/dsPIC30/dsPIC33
* Compiler:           XC8/XC16
* Company:            Microchip Technology, Inc.
*
* Software License Agreement
*
* The software supplied herewith by Microchip Technology Incorporated
* (the "Company") for its PICmicro(R) Microcontroller is intended and
* supplied to you, the Company's customer, for use solely and
* exclusively on Microchip PICmicro Microcontroller products. The
* software is owned by the Company and/or its supplier, and is
* protected under applicable copyright laws. All rights are reserved.
* Any use in violation of the foregoing restrictions may subject the
* user to criminal sanctions under applicable laws, as well as to
* civil liability for the breach of the terms and conditions of this
* license.
*
* THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
* TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
* IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
* CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
*
********************************************************************/

/********************************************************************/
/*                 Definitions to enable SPI channels               */
/********************************************************************/

// Enable SPI channel 1
//#define DRV_SPI_CONFIG_CHANNEL_1_ENABLE
// Enable SPI channel 2
#define DRV_SPI_CONFIG_CHANNEL_2_ENABLE
// Enable SPI channel 3
//#define DRV_SPI_CONFIG_CHANNEL_3_ENABLE
// Enable SPI channel 4
//#define DRV_SPI_CONFIG_CHANNEL_4_ENABLE


// Disable SPI FIFO mode in parts where it is not supported
// Enabling the FIFO mode will improve library performance.
// In this demo this definition is sometimes disabled because early versions of the PIC24FJ128GA010s have an errata preventing this feature from being used.
#if defined (__XC8__) || defined (__PIC24FJ128GA010__)
#define DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
#endif

#define DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE // not using FIFO at the moment
        















