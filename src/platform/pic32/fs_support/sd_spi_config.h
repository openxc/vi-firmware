/******************************************************************************
*
*                        Microchip File I/O Library
*
******************************************************************************
* FileName:           sd_spi_config.h
* Processor:          PIC24/dsPIC30/dsPIC33
* Compiler:           XC16
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

//-------------Function name redirects------------------------------------------
// During the media initialization sequence for SD cards, it is
// necessary to clock the media at a frequency between 100 kHz and 400 kHz,
// since some media types power up in open drain output mode and cannot run
// fast initially.
// On PIC18 devices, when the CPU is running at full frequency, the standard SPI
// prescalars cannot reach a low enough SPI frequency.  Therefore, we provide these
// configuration options to allow the user to remap the SPI functions called during
// the "slow" part of the initialization to user-implemented functions that can
// provide the correct functionality.  For example, a bit-banged SPI module could
// be implemented to provide a clock between 100 and 400 kHz.

// For PIC18 versions of this demo, the slow functions are mapped to the fast SPI driver functions.
// The slower clock frequency will be achieved by disabling the PLL during the
// SD Card initialization.  Note that the SYS_CLK_FrequencySystemGet function 
// was implemented to return the correct clock frequency when using both a PLL-enabled 
// or PLL-disabled clock.

#if defined (__XC8)
    // Define the function to initialize the SPI module for operation at a slow clock rate
    #define FILEIO_SD_SPIInitialize_Slow    FILEIO_SD_SPISlowInitialize
    // Define the function to send a media command at a slow clock rate
    #define FILEIO_SD_SendMediaCmd_Slow     FILEIO_SD_SendCmdSlow
    // Define the function to write an SPI byte at a slow clock rate
    #define FILEIO_SD_SPI_Put_Slow          DRV_SPI_Put
    // Define the function to read an SPI byte at a slow clock rate
    #define FILEIO_SD_SPI_Get_Slow          DRV_SPI_Get
#else
    // Define the function to initialize the SPI module for operation at a slow clock rate
    #define FILEIO_SD_SPIInitialize_Slow    FILEIO_SD_SPISlowInitialize
    // Define the function to send a media command at a slow clock rate
    #define FILEIO_SD_SendMediaCmd_Slow     FILEIO_SD_SendCmd
    // Define the function to write an SPI byte at a slow clock rate
    #define FILEIO_SD_SPI_Put_Slow          DRV_SPI_Put
    // Define the function to read an SPI byte at a slow clock rate
    #define FILEIO_SD_SPI_Get_Slow          DRV_SPI_Get
#endif

// Uncomment FILEIO_SD_CONFIG_MEDIA_SOFT_DETECT to enable soft detect of an SD card.
// Some connectors do not have a card detect pin and must use software to detect
// the presence of a card.
#ifdef CROSSCHASM_CELLULAR_C5
	
#else
	#define FILEIO_SD_CONFIG_MEDIA_SOFT_DETECT
#endif

#define SPI_FREQUENCY 12000000
