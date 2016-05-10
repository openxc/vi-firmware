/******************************************************************************
 *
 *                Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    None
 * Processor:       PIC18/PIC24/dsPIC30/dsPIC33/PIC32
 * Compiler:        C18/C30/C32
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
*****************************************************************************/


#ifndef _HARDWAREPROFILE_H_
#define _HARDWAREPROFILE_H_



#define MEDIA_SOFT_DETECT
// Define your clock speed here

// Sample clock speed for PIC18
#if defined (__18CXX)

    #define GetSystemClock()        40000000                        // System clock frequency (Hz)
    #define GetPeripheralClock()    GetSystemClock()                // Peripheral clock freq.
    #define GetInstructionClock()   (GetSystemClock() / 4)          // Instruction clock freq.

// Sample clock speed for a 16-bit processor
#elif defined (__C30__)

    #define GetSystemClock()        32000000
    #define GetPeripheralClock()    GetSystemClock()
    #define GetInstructionClock()   (GetSystemClock() / 2)

    // Clock values
    #define MILLISECONDS_PER_TICK       10                      // Definition for use with a tick timer
    #define TIMER_PRESCALER             TIMER_PRESCALER_8       // Definition for use with a tick timer
    #define TIMER_PERIOD                20000                   // Definition for use with a tick timer

// Sample clock speed for a 32-bit processor
#elif defined (__PIC32MX__)

    // Indicates that the PIC32 clock is running at 48 MHz
    //#define RUN_AT_48MHZ
    // Indicates that the PIC32 clock is running at 24 MHz
    //#define RUN_AT_24MHZ
    // Indicates that the PIC32 clock is running at 80 MHz
    #define RUN_AT_80MHZ

    // Various clock values

    #if defined(RUN_AT_48MHZ)
        #define GetSystemClock()            48000000UL              // System clock frequency (Hz)
        #define GetPeripheralClock()        48000000UL              // Peripheral clock frequency
        #define GetInstructionClock()       (GetSystemClock())      // Instruction clock frequency
    #elif defined(RUN_AT_24MHZ)
        #define GetSystemClock()            24000000UL
        #define GetPeripheralClock()        24000000UL
        #define GetInstructionClock()       (GetSystemClock())
    #elif defined(RUN_AT_80MHZ)    
        #define GetSystemClock()            (80000000ul)
        #define GetPeripheralClock()        (GetSystemClock()/2) 
        #define GetInstructionClock()       (GetSystemClock())
    #else
        #error Choose a speed
    #endif

    // Clock values

    #define MILLISECONDS_PER_TICK       10                  // Definition for use with a tick timer
    #define TIMER_PRESCALER             TIMER_PRESCALER_8   // Definition for use with a tick timer
    #define TIMER_PERIOD                37500               // Definition for use with a tick timer
#endif
    




// Select your interface type
// This library currently only supports a single physical interface layer at a time


// Description: Macro used to enable the SD-SPI physical layer (SD-SPI.c and .h)
#define USE_SD_INTERFACE_WITH_SPI

// Description: Macro used to enable the CF-PMP physical layer (CF-PMP.c and .h)
//#define USE_CF_INTERFACE_WITH_PMP

// Description: Macro used to enable the CF-Manual physical layer (CF-Bit transaction.c and .h)                                                                
//#define USE_MANUAL_CF_INTERFACE

// Description: Macro used to enable the USB Host physical layer (USB host MSD library)
//#define USE_USB_INTERFACE


/*********************************************************************/
/******************* Pin and Register Definitions ********************/
/*********************************************************************/

/* SD Card definitions: Change these to fit your application when using
   an SD-card-based physical layer                                   */

#ifdef USE_SD_INTERFACE_WITH_SPI
    #ifdef __18CXX
    
        // Sample definition for PIC18 (modify to fit your own project)

        // Description: SD-SPI Chip Select Output bit
        #define SD_CS               LATBbits.LATB3
        // Description: SD-SPI Chip Select TRIS bit
        #define SD_CS_TRIS          TRISBbits.TRISB3
        
        // Description: SD-SPI Card Detect Input bit
        #define SD_CD               PORTBbits.RB4
        // Description: SD-SPI Card Detect TRIS bit
        #define SD_CD_TRIS          TRISBbits.TRISB4
        
        // Description: SD-SPI Write Protect Check Input bit
        #define SD_WE               PORTAbits.RA4
        // Description: SD-SPI Write Protect Check TRIS bit
        #define SD_WE_TRIS          TRISAbits.TRISA4
        
        // Registers for the SPI module you want to use

        // Description: The main SPI control register
        #define SPICON1             SSP1CON1
        // Description: The SPI status register
        #define SPISTAT             SSP1STAT
        // Description: The SPI buffer
        #define SPIBUF              SSP1BUF
        // Description: The receive buffer full bit in the SPI status register
        #define SPISTAT_RBF         SSP1STATbits.BF
        // Description: The bitwise define for the SPI control register (i.e. _____bits)
        #define SPICON1bits         SSP1CON1bits
        // Description: The bitwise define for the SPI status register (i.e. _____bits)
        #define SPISTATbits         SSP1STATbits

        // Description: The interrupt flag for the SPI module
        #define SPI_INTERRUPT_FLAG  PIR1bits.SSPIF   
        // Description: The enable bit for the SPI module
        #define SPIENABLE           SPICON1bits.SSPEN

/*
        // Defines for the FS-USB demo board

        // Tris pins for SCK/SDI/SDO lines
        #define SPICLOCK            TRISBbits.TRISB1
        #define SPIIN               TRISBbits.TRISB0
        #define SPIOUT              TRISCbits.TRISC7

        // Latch pins for SCK/SDI/SDO lines
        #define SPICLOCKLAT         LATBbits.LATB1
        #define SPIINLAT            LATBbits.LATB0
        #define SPIOUTLAT           LATCbits.LATC7

        // Port pins for SCK/SDI/SDO lines
        #define SPICLOCKPORT        PORTBbits.RB1
        #define SPIINPORT           PORTBbits.RB0
        #define SPIOUTPORT          PORTCbits.RC7
*/

        // Defines for the HPC Explorer board

        // Description: The TRIS bit for the SCK pin
        #define SPICLOCK            TRISCbits.TRISC3
        // Description: The TRIS bit for the SDI pin
        #define SPIIN               TRISCbits.TRISC4
        // Description: The TRIS bit for the SDO pin
        #define SPIOUT              TRISCbits.TRISC5

        // Description: The output latch for the SCK pin
        #define SPICLOCKLAT         LATCbits.LATC3
        // Description: The output latch for the SDI pin
        #define SPIINLAT            LATCbits.LATC4
        // Description: The output latch for the SDO pin
        #define SPIOUTLAT           LATCbits.LATC5

        // Description: The port for the SCK pin
        #define SPICLOCKPORT        PORTCbits.RC3
        // Description: The port for the SDI pin
        #define SPIINPORT           PORTCbits.RC4
        // Description: The port for the SDO pin
        #define SPIOUTPORT          PORTCbits.RC5


        // Will generate an error if the clock speed is too low to interface to the card
        #if (GetSystemClock() < 400000)
            #error System clock speed must exceed 400 kHz
        #endif

    #elif defined __PIC24F__

        // Description: SD-SPI Chip Select Output bit
        #define SD_CS                LATBbits.LATB1
        // Description: SD-SPI Chip Select TRIS bit
        #define SD_CS_TRIS            TRISBbits.TRISB1
        
        // Description: SD-SPI Card Detect Input bit
        #define SD_CD               PORTFbits.RF0
        // Description: SD-SPI Card Detect TRIS bit
        #define SD_CD_TRIS          TRISFbits.TRISF0
        
        // Description: SD-SPI Write Protect Check Input bit
        #define SD_WE               PORTFbits.RF1
        // Description: SD-SPI Write Protect Check TRIS bit
        #define SD_WE_TRIS          TRISFbits.TRISF1
        
        // Registers for the SPI module you want to use

        // Description: The main SPI control register
        #define SPICON1             SPI1CON1
        // Description: The SPI status register
        #define SPISTAT             SPI1STAT
        // Description: The SPI Buffer
        #define SPIBUF              SPI1BUF
        // Description: The receive buffer full bit in the SPI status register
        #define SPISTAT_RBF         SPI1STATbits.SPIRBF
        // Description: The bitwise define for the SPI control register (i.e. _____bits)
        #define SPICON1bits         SPI1CON1bits
        // Description: The bitwise define for the SPI status register (i.e. _____bits)
        #define SPISTATbits         SPI1STATbits
        // Description: The enable bit for the SPI module
        #define SPIENABLE           SPISTATbits.SPIEN

        // Tris pins for SCK/SDI/SDO lines

        // Description: The TRIS bit for the SCK pin
        #define SPICLOCK            TRISFbits.TRISF6
        // Description: The TRIS bit for the SDI pin
        #define SPIIN               TRISFbits.TRISF7
        // Description: The TRIS bit for the SDO pin
        #define SPIOUT              TRISFbits.TRISF8

        // Will generate an error if the clock speed is too low to interface to the card
        #if (GetSystemClock() < 100000)
            #error Clock speed must exceed 100 kHz
        #endif    

    #elif defined (__PIC32MX__)
        // Registers for the SPI module you want to use
       // #define MDD_USE_SPI_1
        #define MDD_USE_SPI_2

        //SPI Configuration
        #define SPI_START_CFG_1     (PRI_PRESCAL_64_1 | SEC_PRESCAL_8_1 | MASTER_ENABLE_ON | SPI_CKE_ON | SPI_SMP_ON)
        #define SPI_START_CFG_2     (SPI_ENABLE)

        // Define the SPI frequency
        #define SPI_FREQUENCY            (8000000)
    
        #if defined MDD_USE_SPI_1
            // Description: SD-SPI Chip Select Output bit
            #define SD_CS               LATBbits.LATB1
            // Description: SD-SPI Chip Select TRIS bit
            #define SD_CS_TRIS          TRISBbits.TRISB1
            
            // Description: SD-SPI Card Detect Input bit
            #define SD_CD               PORTFbits.RF0
            // Description: SD-SPI Card Detect TRIS bit
            #define SD_CD_TRIS          TRISFbits.TRISF0

            // Description: SD-SPI Write Protect Check Input bit
            #define SD_WE               PORTFbits.RF1
            // Description: SD-SPI Write Protect Check TRIS bit
            #define SD_WE_TRIS          TRISFbits.TRISF1
                   
            // Description: The main SPI control register
            #define SPICON1             SPI1CON
            // Description: The SPI status register
            #define SPISTAT             SPI1STAT
            // Description: The SPI Buffer
            #define SPIBUF              SPI1BUF
            // Description: The receive buffer full bit in the SPI status register
            #define SPISTAT_RBF         SPI1STATbits.SPIRBF
            // Description: The bitwise define for the SPI control register (i.e. _____bits)
            #define SPICON1bits         SPI1CONbits
            // Description: The bitwise define for the SPI status register (i.e. _____bits)
            #define SPISTATbits         SPI1STATbits
            // Description: The enable bit for the SPI module
            #define SPIENABLE           SPICON1bits.ON
            // Description: The definition for the SPI baud rate generator register (PIC32)
            #define SPIBRG                SPI1BRG

            // Tris pins for SCK/SDI/SDO lines

            // Description: The TRIS bit for the SCK pin
            #define SPICLOCK            TRISFbits.TRISF6
            // Description: The TRIS bit for the SDI pin
            #define SPIIN               TRISFbits.TRISF7
            // Description: The TRIS bit for the SDO pin
            #define SPIOUT              TRISFbits.TRISF8
            //SPI library functions
            #define putcSPI             putcSPI1
            #define getcSPI             getcSPI1
            #define OpenSPI(config1, config2)   OpenSPI1(config1, config2)
            
        #elif defined MDD_USE_SPI_2
            // Description: SD-SPI Chip Select Output bit
            #define SD_CS               LATGbits.LATG9
            // Description: SD-SPI Chip Select TRIS bit
            #define SD_CS_TRIS          TRISGbits.TRISG9
            
            // Description: SD-SPI Card Detect Input bit --NC
            #define SD_CD               PORTBbits.RB6
            // Description: SD-SPI Card Detect TRIS bit
            #define SD_CD_TRIS          TRISBbits.TRISB6

            // Description: SD-SPI Write Protect Check Input bit --NC
            #define SD_WE               PORTBbits.RB1    
            // Description: SD-SPI Write Protect Check TRIS bit
            #define SD_WE_TRIS          TRISBbits.TRISB1
            
            // Description: The main SPI control register
            #define SPICON1             SPI2CON
            // Description: The SPI status register
            #define SPISTAT             SPI2STAT
            // Description: The SPI Buffer
            #define SPIBUF              SPI2BUF
            // Description: The receive buffer full bit in the SPI status register
            #define SPISTAT_RBF         SPI2STATbits.SPIRBF
            // Description: The bitwise define for the SPI control register (i.e. _____bits)
            #define SPICON1bits         SPI2CONbits
            // Description: The bitwise define for the SPI status register (i.e. _____bits)
            #define SPISTATbits         SPI2STATbits
            // Description: The enable bit for the SPI module
            #define SPIENABLE           SPI2CONbits.ON
            // Description: The definition for the SPI baud rate generator register (PIC32)
            #define SPIBRG                SPI2BRG

            // Tris pins for SCK/SDI/SDO lines

            // Description: The TRIS bit for the SCK pin
            #define SPICLOCK            TRISGbits.TRISG6
            // Description: The TRIS bit for the SDI pin
            #define SPIIN               TRISGbits.TRISG7
            // Description: The TRIS bit for the SDO pin
            #define SPIOUT              TRISGbits.TRISG8
            //SPI library functions
            #define putcSPI             putcSPI2
            #define getcSPI             getcSPI2
            #define OpenSPI(config1, config2)   OpenSPI2(config1, config2)
        #endif       
        // Will generate an error if the clock speed is too low to interface to the card
        #if (GetSystemClock() < 100000)
            #error Clock speed must exceed 100 kHz
        #endif    
    
    #endif

#endif


#ifdef USE_CF_INTERFACE_WITH_PMP

    /* CompactFlash-PMP card definitions: change these to fit your application when
    using the PMP module to interface with CF cards                                */
    
    #ifdef __18CXX
        #error The PIC18 architecture does not currently support PMP interface to CF cards
    #elif defined __dsPIC30F__
    
        // Sample dsPIC30 defines
        
        // Description: The output latch for the CF Reset signal
        #define CF_PMP_RST            _RD0
        // Description: The TRIS bit for the CF Reset signal
        #define CF_PMP_RESETDIR        _TRISD0
        // Description: The input port for the CF Ready signal
        #define CF_PMP_RDY             _RD12
        // Description: The TRIS bit for the CF Ready signal
        #define CF_PMP_READYDIR        _TRISD12
        // Description: The input port for the CF card detect signal
        #define CF_PMP_CD1            _RC4
        // Description: The TRIS bit for the CF card detect signal
        #define CF_PMP_CD1DIR        _TRISC4
    
    #elif defined __dsPIC33F__
    
        // Sample dsPIC33 defines

        // Description: The output latch for the CF Reset signal
        #define CF_PMP_RST            _RD0
        // Description: The TRIS bit for the CF Reset signal
        #define CF_PMP_RESETDIR        _TRISD0
        // Description: The input port for the CF Ready signal
        #define CF_PMP_RDY             _RD12
        // Description: The TRIS bit for the CF Ready signal
        #define CF_PMP_READYDIR        _TRISD12
        // Description: The input port for the CF card detect signal
        #define CF_PMP_CD1            _RC4
        // Description: The TRIS bit for the CF card detect signal
        #define CF_PMP_CD1DIR        _TRISC4
    
    #elif defined __PIC24F__
    
        // Default case for PIC24F

        // Description: The output latch for the CF Reset signal
        #define CF_PMP_RST            LATDbits.LATD0
        // Description: The TRIS bit for the CF Reset signal
        #define CF_PMP_RESETDIR        TRISDbits.TRISD0
        // Description: The input port for the CF Ready signal
        #define CF_PMP_RDY             PORTDbits.RD12
        // Description: The TRIS bit for the CF Ready signal
        #define CF_PMP_READYDIR        TRISDbits.TRISD12
        // Description: The input port for the CF card detect signal
        #define CF_PMP_CD1            PORTCbits.RC4
        // Description: The TRIS bit for the CF card detect signal
        #define CF_PMP_CD1DIR        TRISCbits.TRISC4
    
    #endif
    
    // Description: Defines the PMP data bus direction register
    #define MDD_CFPMP_DATADIR       TRISE
#endif


#ifdef USE_MANUAL_CF_INTERFACE
    // Use these definitions with CF-Bit transaction.c and .h
    // This will manually perform parallel port transactions

    #ifdef __18CXX
    
        // Address lines

        // Description: The CF address bus output latch register (for PIC18)
        #define ADDBL                   LATA
        // Description: The CF address bus TRIS register (for PIC18)
        #define ADDDIR                  TRISA
        
        // Data bus

        // Description: The Manual CF data bus port register
        #define MDD_CFBT_DATABIN        PORTD
        // Description: The Manual CF data bus output latch register
        #define MDD_CFBT_DATABOUT       LATD
        // Description: The Manual CF data bus TRIS register
        #define MDD_CFBT_DATADIR        TRISD

        // control bus lines

        // Description: The CF card chip select output latch bit
        #define CF_CE                   LATEbits.LATE1
        // Description: The CF card chip select TRIS bit
        #define CF_CEDIR                TRISEbits.TRISE1
        // Description: The CF card output enable strobe latch bit
        #define CF_OE                   LATAbits.LATA5
        // Description: The CF card output enable strobe TRIS bit
        #define CF_OEDIR                TRISAbits.TRISA5
        // Description: The CF card write enable strobe latch bit
        #define CF_WE                   LATAbits.LATA4
        // Description: The CF card write enable strobe TRIS bit
        #define CF_WEDIR                TRISAbits.TRISA4
        // Description: The CF card reset signal latch bit
        #define CF_BT_RST               LATEbits.LATE0
        // Description: The CF card reset signal TRIS bit
        #define CF_BT_RESETDIR          TRISEbits.TRISE0
        // Description: The CF card ready signal port bit
        #define CF_BT_RDY               PORTEbits.RE2
        // Description: The CF card ready signal TRIS bit
        #define CF_BT_READYDIR          TRISEbits.TRISE2
        // Description: The CF card detect signal port bit
        #define CF_BT_CD1               PORTCbits.RC2
        // Description: The CF card detect signal TRIS bit
        #define CF_BT_CD1DIR            TRISCbits.TRISC2
    
    #elif defined __dsPIC30F__
    
        // Address lines

         // Description: The CF address bus bit 0 output latch definition (for PIC24/30/33/32)
        #define ADDR0                   _LATB15
        // Description: The CF address bus bit 1 output latch definition (for PIC24/30/33/32)
        #define    ADDR1                   _LATB14
        // Description: The CF address bus bit 2 output latch definition (for PIC24/30/33/32)
        #define ADDR2                   _LATG9
        // Description: The CF address bus bit 3 output latch definition (for PIC24/30/33/32)
        #define ADDR3                   _LATG8
        // Description: The CF address bus bit 0 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS0                _TRISB15
        // Description: The CF address bus bit 1 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS1                _TRISB14
        // Description: The CF address bus bit 2 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS2                _TRISG9
        // Description: The CF address bus bit 3 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS3                _TRISG8
        
        // Data bus

        // Description: The Manual CF data bus port register
        #define MDD_CFBT_DATABIN        PORTE
        // Description: The Manual CF data bus output latch register
        #define MDD_CFBT_DATABOUT       PORTE
        // Description: The Manual CF data bus TRIS register
        #define MDD_CFBT_DATADIR        TRISE
        
        // control bus lines

        // Description: The CF card chip select output latch bit
        #define CF_CE                   _RD11
        // Description: The CF card chip select TRIS bit
        #define CF_CEDIR                _TRISD11
        // Description: The CF card output enable strobe latch bit
        #define CF_OE                   _RD5
        // Description: The CF card output enable strobe TRIS bit
        #define CF_OEDIR                _TRISD5    
        // Description: The CF card write enable strobe latch bit
        #define CF_WE                   _RD4
        // Description: The CF card write enable strobe TRIS bit
        #define CF_WEDIR                _TRISD4
        // Description: The CF card reset signal latch bit
        #define CF_BT_RST               _RD0
        // Description: The CF card reset signal TRIS bit
        #define CF_BT_RESETDIR          _TRISD0
        // Description: The CF card ready signal port bit
        #define CF_BT_RDY               _RD12
        // Description: The CF card ready signal TRIS bit
        #define CF_BT_READYDIR          _TRISD12
        // Description: The CF card detect signal port bit
        #define CF_BT_CD1               _RC4
        // Description: The CF card detect signal TRIS bit
        #define CF_BT_CD1DIR            _TRISC4

    #elif defined __dsPIC33F__

        // Address lines

        // Description: The CF address bus bit 0 output latch definition (for PIC24/30/33/32)
        #define ADDR0                   _LATB15
        // Description: The CF address bus bit 1 output latch definition (for PIC24/30/33/32)
        #define    ADDR1                   _LATB14
        // Description: The CF address bus bit 2 output latch definition (for PIC24/30/33/32)
        #define ADDR2                   _LATG9
        // Description: The CF address bus bit 3 output latch definition (for PIC24/30/33/32)
        #define ADDR3                   _LATG8
        // Description: The CF address bus bit 0 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS0                _TRISB15
        // Description: The CF address bus bit 1 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS1                _TRISB14
        // Description: The CF address bus bit 2 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS2                _TRISG9
        // Description: The CF address bus bit 3 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS3                _TRISG8
        
        // Data bus

        // Description: The Manual CF data bus port register
        #define MDD_CFBT_DATABIN        PORTE
        // Description: The Manual CF data bus output latch register
        #define MDD_CFBT_DATABOUT       PORTE
        // Description: The Manual CF data bus TRIS register
        #define MDD_CFBT_DATADIR        TRISE
        
        // control bus lines

        // Description: The CF card chip select output latch bit
        #define CF_CE                   _RD11
        // Description: The CF card chip select TRIS bit
        #define CF_CEDIR                _TRISD11
        // Description: The CF card output enable strobe latch bit
        #define CF_OE                   _RD5
        // Description: The CF card output enable strobe TRIS bit
        #define CF_OEDIR                _TRISD5    
        // Description: The CF card write enable strobe latch bit
        #define CF_WE                   _RD4
        // Description: The CF card write enable strobe TRIS bit
        #define CF_WEDIR                _TRISD4
        // Description: The CF card reset signal latch bit
        #define CF_BT_RST               _RD0
        // Description: The CF card reset signal TRIS bit
        #define CF_BT_RESETDIR          _TRISD0
        // Description: The CF card ready signal port bit
        #define CF_BT_RDY               _RD12
        // Description: The CF card ready signal TRIS bit
        #define CF_BT_READYDIR          _TRISD12
        // Description: The CF card detect signal port bit
        #define CF_BT_CD1               _RC4
        // Description: The CF card detect signal TRIS bit
        #define CF_BT_CD1DIR            _TRISC4
    
    #elif defined __PIC24F__
    
        // Address lines

        // Description: The CF address bus bit 0 output latch definition (for PIC24/30/33/32)
        #define ADDR0                   LATBbits.LATB15
        // Description: The CF address bus bit 1 output latch definition (for PIC24/30/33/32)
        #define    ADDR1                   LATBbits.LATB14
        // Description: The CF address bus bit 2 output latch definition (for PIC24/30/33/32)
        #define ADDR2                   LATGbits.LATG9
        // Description: The CF address bus bit 3 output latch definition (for PIC24/30/33/32)
        #define ADDR3                   LATGbits.LATG8
        // Description: The CF address bus bit 0 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS0                TRISBbits.TRISB15
        // Description: The CF address bus bit 1 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS1                TRISBbits.TRISB14
        // Description: The CF address bus bit 2 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS2                TRISGbits.TRISG9
        // Description: The CF address bus bit 3 TRIS definition (for PIC24/30/33/32)
        #define ADRTRIS3                TRISGbits.TRISG8
        
        // Data bus

        // Description: The Manual CF data bus port register
        #define MDD_CFBT_DATABIN        PORTE
        // Description: The Manual CF data bus output latch register
        #define MDD_CFBT_DATABOUT       PORTE
        // Description: The Manual CF data bus TRIS register
        #define MDD_CFBT_DATADIR        TRISE
        
        // control bus lines

        // Description: The CF card chip select output latch bit
        #define CF_CE                   PORTDbits.RD11
        // Description: The CF card chip select TRIS bit
        #define CF_CEDIR                TRISDbits.TRISD11
        // Description: The CF card output enable strobe latch bit
        #define CF_OE                   PORTDbits.RD5
        // Description: The CF card output enable strobe TRIS bit
        #define CF_OEDIR                TRISDbits.TRISD5    
        // Description: The CF card write enable strobe latch bit
        #define CF_WE                   PORTDbits.RD4
        // Description: The CF card write enable strobe TRIS bit
        #define CF_WEDIR                TRISDbits.TRISD4
        // Description: The CF card reset signal latch bit
        #define CF_BT_RST               PORTDbits.RD0
        // Description: The CF card reset signal TRIS bit
        #define CF_BT_RESETDIR          TRISDbits.TRISD0
        // Description: The CF card ready signal port bit
        #define CF_BT_RDY               PORTDbits.RD12
        // Description: The CF card ready signal TRIS bit
        #define CF_BT_READYDIR          TRISDbits.TRISD12
        // Description: The CF card detect signal port bit
        #define CF_BT_CD1               PORTCbits.RC4
        // Description: The CF card detect signal TRIS bit
        #define CF_BT_CD1DIR            TRISCbits.TRISC4
    #endif
#endif

#include <uart2.h>

#endif
