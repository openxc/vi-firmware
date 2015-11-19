// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright 2015 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/
//DOM-IGNORE-END

#ifndef SDMMC_H
#define SDMMC_H

#include "system_config.h"
#include "system.h"
#include <stdint.h>
#include <stdbool.h>


/*****************************************************************************/
/*                        Custom structures and definitions                  */
/*****************************************************************************/
//Definition for a structure used when calling either FILEIO_SD_AsyncReadTasks()
//function, or the FILEIO_SD_AsyncWriteTasks() function.
typedef struct
{
    uint16_t wNumBytes;         //Number of bytes to attempt to read or write in the next call to MDD_SDSPI_AsyncReadTasks() or MDD_SDSPI_AsyncWriteTasks.  May be updated between calls to the handler.
    uint32_t dwBytesRemaining; //Should be initialized to the total number of uint8_ts that you wish to read or write.  This value is allowed to be greater than a single block size of the media.
    uint8_t* pBuffer;          //Pointer to where the read/written uint8_ts should be copied to/from.  May be updated between calls to the handler function.
    uint32_t dwAddress;        //Starting block address to read or to write to on the media.  Should only get initialized, do not modify after that.
    uint8_t bStateVariable;    //State machine variable.  Should get initialized to ASYNC_READ_QUEUED or ASYNC_WRITE_QUEUED to start an operation.  After that, do not modify until the read or write is complete.
}FILEIO_SD_ASYNC_IO;

//Response codes for the FILEIO_SD_AsyncReadTasks() function.
#define FILEIO_SD_ASYNC_READ_COMPLETE             0x00
#define FILEIO_SD_ASYNC_READ_BUSY                 0x01
#define FILEIO_SD_ASYNC_READ_NEW_PACKET_READY     0x02
#define FILEIO_SD_ASYNC_READ_ERROR                0xFF

//FILEIO_SD_AsyncReadTasks() state machine variable values. These are used internally to sd_spi.c.
#define FILEIO_SD_ASYNC_READ_COMPLETE             0x00
#define FILEIO_SD_ASYNC_READ_QUEUED               0x01    //Initialize to this to start a read sequence
#define FILEIO_SD_ASYNC_READ_WAIT_START_TOKEN     0x03
#define FILEIO_SD_ASYNC_READ_NEW_PACKET_READY     0x02
#define FILEIO_SD_ASYNC_READ_ABORT                0xFE
#define FILEIO_SD_ASYNC_READ_ERROR                0xFF

//Possible return values when calling FILEIO_SD_AsyncWriteTasks()
#define FILEIO_SD_ASYNC_WRITE_COMPLETE        0x00
#define FILEIO_SD_ASYNC_WRITE_SEND_PACKET     0x02
#define FILEIO_SD_ASYNC_WRITE_BUSY            0x03
#define FILEIO_SD_ASYNC_WRITE_ERROR           0xFF

//FILEIO_SD_AsyncWriteTasks() state machine variable values. These are used internally to sd_spi.c.
#define FILEIO_SD_ASYNC_WRITE_COMPLETE            0x00
#define FILEIO_SD_ASYNC_WRITE_QUEUED              0x01    //Initialize to this to start a write sequence
#define FILEIO_SD_ASYNC_WRITE_TRANSMIT_PACKET     0x02
#define FILEIO_SD_ASYNC_WRITE_MEDIA_BUSY          0x03
#define FILEIO_SD_ASYNC_STOP_TOKEN_SENT_WAIT_BUSY 0x04
#define FILEIO_SD_ASYNC_WRITE_ABORT               0xFE
#define FILEIO_SD_ASYNC_WRITE_ERROR               0xFF


/*****************************************************************************/
/*                         Function pointer types                            */
/*****************************************************************************/

/*************************************************************************************
    Function:
        typedef void (*FILEIO_SD_CSSet)(uint8_t value)
        
    Summary:
        Prototype for a user-implemented function to set or 
        clear the SPI's chip select pin.

    Input:
        value - The value of the chip select pin (1 or 0)

    Return Values:
        None

    Description:
        Most functions in this driver require the user to implement the functions 
        that comprise a FILEIO_SD_DRIVE_CONFIG structure.  This function pointer 
        definition describes a function in this structure that will set/clear the 
        chip select pin.
    
    Remarks:
        None                                                  
  ***********************************************************************************/
typedef void (*FILEIO_SD_CSSet)(uint8_t value);

/*************************************************************************************
    Function:
        typedef bool (*FILEIO_SD_CDGet)(void);
        
    Summary:
        Prototype for a user-implemented function to get the current state of the  
        Card Detect pin, if one exists.

    Input:
        None

    Return Values:
        true if the card detect pin indicates the card is inserted, false otherwise.
        If the physical socket in use does not have a card detect pin, this 
        function should always return true.

    Description:
        Most functions in this driver require the user to implement the functions 
        that comprise a FILEIO_SD_DRIVE_CONFIG structure.  This function pointer 
        definition describes a function in this structure that will return the value 
        of a card detect pin.  These pins are a typical feature on the physical 
        sockets manufactured for SD card (not on the SD cards themselves).  On 
        some types of SD card (i.e. micro SD) this pin will not be available.
    
    Remarks:
        None                                                  
  ***********************************************************************************/
typedef bool (*FILEIO_SD_CDGet)(void);

/*************************************************************************************
    Function:
        typedef bool (*FILEIO_SD_WPGet)(void);
        
    Summary:
        Prototype for a user-implemented function to get the current state of the  
        Write Protect pin, if one exists.

    Input:
        None

    Return Values:
        true if the write protect pin indicates the card is inserted, false otherwise.
        If the physical socket in use does not have a write protect pin, this 
        function should always return false.

    Description:
        Most functions in this driver require the user to implement the functions 
        that comprise a FILEIO_SD_DRIVE_CONFIG structure.  This function pointer 
        definition describes a function in this structure that will return the value 
        of a write protect pin.  These pins are a typical feature on the physical 
        sockets manufactured for SD card (not on the SD cards themselves).  On 
        some types of SD card (i.e. micro SD) this pin will not be available.
    
    Remarks:
        None                                                  
  ***********************************************************************************/
typedef bool (*FILEIO_SD_WPGet)(void);

/*************************************************************************************
    Function:
        typedef void (*FILEIO_SD_PinConfigure)(void);
        
    Summary:
        Prototype for a user-implemented function to configure the pins used by 
        the SD card.

    Input:
        None

    Return Values:
        None.

    Description:
        Most functions in this driver require the user to implement the functions 
        that comprise a FILEIO_SD_DRIVE_CONFIG structure.  This function pointer 
        definition describes a function in this structure that will configure all of 
        the pins used by the SD Card.  The configuration may involve setting/clearing
        the TRIS bits, disabling the analog state of the pins, setting up peripheral 
        pin select, or other operations (depending on the device).
        The user must configure the chip select, card detect, and write protect pins.
        Optionally, configuration for the SPI pins (SDI, SDO, SCK) and SPI module 
        may be performed in this function, though it may make more sense to configure 
        those in another part of any given application.
    
    Remarks:
        None                                                  
  ***********************************************************************************/
typedef void (*FILEIO_SD_PinConfigure)(void);

// A configuration structure used by the SD-SPI driver functions to perform specific 
// tasks.
typedef struct
{
    uint8_t index;                                  // The numeric index of the SPI module to use (i.e. 1 for SPI1/SSP1, 2 for SPI2, SSP2,...)
    FILEIO_SD_CSSet csFunc;                         // Pointer to a user-implemented function to set/clear the chip select pins
    FILEIO_SD_CDGet cdFunc;                         // Pointer to a user-implemented function to get the status of the card detect pin
    FILEIO_SD_WPGet wpFunc;                         // Pointer to a user-implemented function to get the status of the write protect pin
    FILEIO_SD_PinConfigure configurePins;           // Pointer to a user-implemented function to configure the pins used by the SD Card
} FILEIO_SD_DRIVE_CONFIG;


/*****************************************************************************/
/*                                 Public Prototypes                         */
/*****************************************************************************/

/*********************************************************
  Function:
    bool FILEIO_SD_MediaDetect (FILEIO_SD_DRIVE_CONFIG * config)
  Summary:
    Determines whether an SD card is present
  Conditions:
    The FILEIO_SD_MediaDetect function pointer must be configured
    to point to this function in FSconfig.h
  Input:
    config - The given drive configuration
  Return Values:
    true -  Card detected
    false - No card detected
  Side Effects:
    None.
  Description:
    The FILEIO_SD_MediaDetect function determine if an SD card is connected to 
    the microcontroller.
    If the MEDIA_SOFT_DETECT is not defined, the detection is done by polling
    the SD card detect pin.
    The MicroSD connector does not have a card detect pin, and therefore a
    software mechanism must be used. To do this, the SEND_STATUS command is sent 
    to the card. If the card is not answering with 0x00, the card is either not 
    present, not configured, or in an error state. If this is the case, we try
    to reconfigure the card. If the configuration fails, we consider the card not 
    present (it still may be present, but malfunctioning). In order to use the 
    software card detect mechanism, the MEDIA_SOFT_DETECT macro must be defined.
    
  Remarks:
    None                                                  
  *********************************************************/
bool FILEIO_SD_MediaDetect(FILEIO_SD_DRIVE_CONFIG * config);

/*****************************************************************************
  Function:
    FILEIO_MEDIA_INFORMATION *  FILEIO_SD_MediaInitialize (void)
  Summary:
    Initializes the SD card.
  Conditions:
    The FILEIO_SD_MediaInitialize function pointer must be pointing to this function.
  Input:
    config - An SD Drive configuration structure pointer
  Return Values:
    The function returns a pointer to the FILEIO_MEDIA_INFORMATION structure.  The
    errorCode member may contain the following values:
        * MEDIA_NO_ERROR - The media initialized successfully
        * MEDIA_CANNOT_INITIALIZE - Cannot initialize the media.  
  Side Effects:
    None.
  Description:
    This function will send initialization commands to and SD card.
  Remarks:
    Psuedo code flow for the media initialization process is as follows:

-------------------------------------------------------------------------------------------
SD Card SPI Initialization Sequence (for physical layer v1.x or v2.0 device) is as follows:
-------------------------------------------------------------------------------------------
0.  Power up tasks
    a.  Initialize microcontroller SPI module to no more than 400kbps rate so as to support MMC devices.
    b.  Add delay for SD card power up, prior to sending it any commands.  It wants the 
        longer of: 1ms, the Vdd ramp time (time from 2.7V to Vdd stable), and 74+ clock pulses.
1.  Send CMD0 (GO_IDLE_STATE) with CS = 0.  This puts the media in SPI mode and software resets the SD/MMC card.
2.  Send CMD8 (SEND_IF_COND).  This requests what voltage the card wants to run at. 
    Note: Some cards will not support this command.
    a.  If illegal command response is received, this implies either a v1.x physical spec device, or not an SD card (ex: MMC).
    b.  If normal response is received, then it must be a v2.0 or later SD memory card.

If v1.x device:
-----------------
3.  Send CMD1 repeatedly, until initialization complete (indicated by R1 response uint8_t/idle bit == 0)
4.  Basic initialization is complete.  May now switch to higher SPI frequencies.
5.  Send CMD9 to read the CSD structure.  This will tell us the total flash size and other info which will be useful later.
6.  Parse CSD structure bits (based on v1.x structure format) and extract useful information about the media.
7.  The card is now ready to perform application data transfers.

If v2.0+ device:
-----------------
3.  Verify the voltage range is feasible.  If not, unusable card, should notify user that the card is incompatible with this host.
4.  Send CMD58 (Read OCR).
5.  Send CMD55, then ACMD41 (SD_SEND_OP_COND, with HCS = 1).
    a.  Loop CMD55/ACMD41 until R1 response uint8_t == 0x00 (indicating the card is no longer busy/no longer in idle state).  
6.  Send CMD58 (Get CCS).
    a.  If CCS = 1 --> SDHC card.
    b.  If CCS = 0 --> Standard capacity SD card (which is v2.0+).
7.  Basic initialization is complete.  May now switch to higher SPI frequencies.
8.  Send CMD9 to read the CSD structure.  This will tell us the total flash size and other info which will be useful later.
9.  Parse CSD structure bits (based on v2.0 structure format) and extract useful information about the media.
10. The card is now ready to perform application data transfers.
--------------------------------------------------------------------------------
********************************************************************************/
FILEIO_MEDIA_INFORMATION * FILEIO_SD_MediaInitialize(FILEIO_SD_DRIVE_CONFIG * config);

/*********************************************************
  Function:
    bool FILEIO_SD_MediaDeinitialize(
        FILEIO_SD_DRIVE_CONFIG * config)
  Summary:
    Disables the SD card
  Conditions:
    The FILEIO_SD_MediaDeinitialize function pointer is pointing 
    towards this function.
  Input:
    config - An SD Drive configuration structure pointer
  Return:
    true if successful, false otherwise
  Side Effects:
    None.
  Description:
    This function will disable the SPI port and deselect
    the SD card.
  Remarks:
    None
  *********************************************************/
bool FILEIO_SD_MediaDeinitialize(FILEIO_SD_DRIVE_CONFIG * config);

/*********************************************************
  Function:
    uint32_t FILEIO_SD_CapacityRead(
        FILEIO_SD_DRIVE_CONFIG * config)
  Summary:
    Determines the current capacity of the SD card
  Conditions:
    FILEIO_SD_MediaInitialize() is complete
  Input:
    config - An SD Drive configuration structure pointer
  Return:
    The capacity of the device
  Side Effects:
    None.
  Description:
    The FILEIO_SD_CapacityRead function is used by the
    USB mass storage class to return the total number
    of sectors on the card.
  Remarks:
    None
  *********************************************************/
uint32_t FILEIO_SD_CapacityRead(FILEIO_SD_DRIVE_CONFIG * config);

/*********************************************************
  Function:
    uint16_t FILEIO_SD_SectorSizeRead(
        FILEIO_SD_DRIVE_CONFIG * config)
  Summary:
    Determines the current sector size on the SD card
  Conditions:
    FILEIO_SD_MediaInitialize() is complete
  Input:
    config - An SD Drive configuration structure pointer
  Return:
    The size of the sectors for the physical media
  Side Effects:
    None.
  Description:
    The FILEIO_SD_SectorSizeRead function is used by the
    USB mass storage class to return the card's sector
    size to the PC on request.
  Remarks:
    None
  *********************************************************/
uint16_t FILEIO_SD_SectorSizeRead(FILEIO_SD_DRIVE_CONFIG * config);

/*********************************************************
  Function:
    void FILEIO_SD_IOInitialize (
        FILEIO_SD_DRIVE_CONFIG * config)
  Summary:
    Initializes the I/O lines connected to the card
  Conditions:
    FILEIO_SD_MediaInitialize() is complete.  The MDD_InitIO
    function pointer is pointing to this function.
  Input:
    config - An SD Drive configuration structure pointer
  Return:
    None
  Side Effects:
    None.
  Description:
    The FILEIO_SD_IOInitialize function initializes the I/O
    pins connected to the SD card.
  Remarks:
    None
  *********************************************************/
void FILEIO_SD_IOInitialize(FILEIO_SD_DRIVE_CONFIG * config);

/*****************************************************************************
  Function:
    uint8_t FILEIO_SD_SectorRead (uint32_t sector_addr, uint8_t * buffer)
  Summary:
    Reads a sector of data from an SD card.
  Conditions:
    The FILEIO_SD_SectorRead function pointer must be pointing towards this function.
  Input:
    config - An SD Drive configuration structure pointer
    sectorAddress - The address of the sector on the card.
    buffer -      The buffer where the retrieved data will be stored.  If
                  buffer is NULL, do not store the data anywhere.
  Return Values:
    true -  The sector was read successfully
    false - The sector could not be read
  Side Effects:
    None
  Description:
    The FILEIO_SD_SectorRead function reads a sector of data uint8_ts (512 uint8_ts) 
    of data from the SD card starting at the sector address and stores them in 
    the location pointed to by 'buffer.'
  Remarks:
    The card expects the address field in the command packet to be a uint8_t address.
    The sector_addr value is converted to a uint8_t address by shifting it left nine
    times (multiplying by 512).
    
    This function performs a synchronous read operation.  In other uint16_ts, this
    function is a blocking function, and will not return until either the data
    has fully been read, or, a timeout or other error occurred.
  ***************************************************************************************/
bool FILEIO_SD_SectorRead(FILEIO_SD_DRIVE_CONFIG * config, uint32_t sector_addr, uint8_t * buffer);

/*****************************************************************************
  Function:
    bool FILEIO_SD_SectorWrite (FILEIO_SD_DRIVE_CONFIG * config,
        uint32_t sector_addr, uint8_t * buffer, uint8_t allowWriteToZero)
  Summary:
    Writes a sector of data to an SD card.
  Conditions:
    The FILEIO_SD_SectorWrite function pointer must be pointing to this function.
  Input:
    config - An SD Drive configuration structure pointer
    sectorAddress -      The address of the sector on the card.
    buffer -           The buffer with the data to write.
    allowWriteToZero -
                     - true -  Writes to the 0 sector (MBR) are allowed
                     - false - Any write to the 0 sector will fail.
  Return Values:
    true -  The sector was written successfully.
    false - The sector could not be written.
  Side Effects:
    None.
  Description:
    The FILEIO_SD_SectorWrite function writes one sector of data (512 uint8_ts) 
    of data from the location pointed to by 'buffer' to the specified sector of 
    the SD card.
  Remarks:
    The card expects the address field in the command packet to be a uint8_t address.
    The sector_addr value is converted to a uint8_t address by shifting it left nine
    times (multiplying by 512).
  ***************************************************************************************/
bool FILEIO_SD_SectorWrite(FILEIO_SD_DRIVE_CONFIG * config, uint32_t sector_addr, uint8_t * buffer, bool allowWriteToZero);

/*******************************************************************************
  Function:
    uint8_t FILEIO_SD_WriteProtectStateGet
  Summary:
    Indicates whether the card is write-protected.
  Conditions:
    The FILEIO_SD_WriteProtectStateGet function pointer must be pointing to this function.
  Input:
    config - An SD Drive configuration structure pointer
  Return Values:
    true -  The card is write-protected
    false - The card is not write-protected
  Side Effects:
    None.
  Description:
    The FILEIO_SD_WriteProtectStateGet function will determine if the SD card is
    write protected by checking the electrical signal that corresponds to the
    physical write-protect switch.
  Remarks:
    None
*******************************************************************************/
bool FILEIO_SD_WriteProtectStateGet(FILEIO_SD_DRIVE_CONFIG * config);


uint8_t FILEIO_SD_AsyncReadTasks(FILEIO_SD_DRIVE_CONFIG * config, FILEIO_SD_ASYNC_IO*);
uint8_t FILEIO_SD_AsyncWriteTasks(FILEIO_SD_DRIVE_CONFIG * config, FILEIO_SD_ASYNC_IO*);



#endif
