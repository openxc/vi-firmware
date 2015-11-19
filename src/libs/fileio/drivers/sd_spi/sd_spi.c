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

#include <xc.h>
#include "system_config.h"
#include "fileio.h"
#include "../src/fileio_private.h"
#include "system.h"
#include "sd_spi.h"
#include "sd_spi_private.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Global Variables
 *****************************************************************************/
#define FILEIO_SD_STATE_NOT_INITIALIZED         0
#define FILEIO_SD_STATE_READY_FOR_COMMAND       1
#define FILEIO_SD_STATE_BUSY                    2

// Description:  Used for the mass-storage library to determine capacity
uint32_t finalLBA;
uint16_t gMediaSectorSize;
uint8_t gSDMode;
static FILEIO_MEDIA_INFORMATION mediaInformation;
static FILEIO_SD_ASYNC_IO ioInfo; //Declared global context, for fast/code efficient access
static uint8_t gSDMediaState = FILEIO_SD_STATE_NOT_INITIALIZED;

// Summary: Table of SD card commands and parameters
// Description: The sdmmc_cmdtable contains an array of SD card commands, the corresponding CRC code, the
//              response type that the card will return, and a parameter indicating whether to expect
//              additional data from the card.
const FILEIO_SD_COMMAND sdmmc_cmdtable[] =
{
    // cmd                      crc     response
    {FILEIO_SD_COMMAND_GO_IDLE_STATE,          0x95,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SEND_OP_COND,           0xF9,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SEND_IF_COND,           0x87,   FILEIO_SD_RESPONSE_R7,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SEND_CSD,               0xAF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SEND_CID,               0x1B,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_STOP_TRANSMISSION,      0xC3,   FILEIO_SD_RESPONSE_R1b,    FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SEND_STATUS,            0xAF,   FILEIO_SD_RESPONSE_R2,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SET_BLOCK_LENGTH,       0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_READ_SINGLE_BLOCK,      0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_READ_MULTI_BLOCK,       0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_WRITE_SINGLE_BLOCK,     0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_WRITE_MULTI_BLOCK,      0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_MORE_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_TAG_SECTOR_START,       0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_TAG_SECTOR_END,         0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_ERASE,                  0xDF,   FILEIO_SD_RESPONSE_R1b,    FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_APP_CMD,                0x73,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_READ_OCR,               0x25,   FILEIO_SD_RESPONSE_R7,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_CRC_ON_OFF,             0x25,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED},
    {FILEIO_SD_COMMAND_SD_SEND_OP_COND,        0xFF,   FILEIO_SD_RESPONSE_R7,     FILEIO_SD_NO_DATA_EXPECTED}, //Actual response is R3, but has same number of uint8_ts as R7.
    {FILEIO_SD_COMMAND_SET_WRITE_BLOCK_ERASE_COUNT, 0xFF,   FILEIO_SD_RESPONSE_R1,     FILEIO_SD_NO_DATA_EXPECTED}
};

/******************************************************************************
 * Prototypes
 *****************************************************************************/
extern void Delayms(uint8_t milliseconds);
FILEIO_SD_RESPONSE FILEIO_SD_SendCmd(FILEIO_SD_DRIVE_CONFIG * config, uint8_t cmd, uint32_t address);

#if defined __XC8__
    FILEIO_SD_RESPONSE FILEIO_SD_SendCmdSlow(FILEIO_SD_DRIVE_CONFIG * config, uint8_t cmd, uint32_t address);
#endif
void FILEIO_SD_SPISlowInitialize(FILEIO_SD_DRIVE_CONFIG * config);

#ifdef __XC32__
/*********************************************************
  Function:
    static inline __attribute__((always_inline)) unsigned char SPICacutateBRG (unsigned int pb_clk, unsigned int spi_clk)
  Summary:
    Calculate the PIC32 SPI BRG value
  Conditions:
    None
  Input:
    pb_clk -  The value of the PIC32 peripheral clock
    spi_clk - The desired baud rate
  Return:
    The corresponding BRG register value.
  Side Effects:
    None.
  Description:
    The SPICalculateBRG function is used to determine an appropriate BRG register value for the PIC32 SPI module.
  Remarks:
    None                                                  
  *********************************************************/

static inline __attribute__((always_inline)) unsigned char SPICalculateBRG(unsigned int pb_clk, unsigned int spi_clk)
{
    unsigned int brg;

    brg = pb_clk / (2 * spi_clk);

    if(pb_clk % (2 * spi_clk))
        brg++;

    if(brg > 0x100)
        brg = 0x100;

    if(brg)
        brg--;

    return (unsigned char) brg;
}
#endif


bool FILEIO_SD_MediaDetect (FILEIO_SD_DRIVE_CONFIG * config)
{
	return true;
    #ifndef FILEIO_SD_CONFIG_MEDIA_SOFT_DETECT
        return (*config->cdFunc)();
    #else
        FILEIO_SD_RESPONSE    response;

        if(gSDMediaState == FILEIO_SD_STATE_BUSY)
        {
            return true;
        }

        //First check if the media has previously been detected and initialized or not.
        if(gSDMediaState == FILEIO_SD_STATE_NOT_INITIALIZED)
        {
            unsigned char timeout;

            //If the SPI module is not enabled, then the media has evidently not
            //been initialized.  Try to send CMD0 and CMD13 to reset the device and
            //get it into SPI mode (if present), and then request the status of
            //the media.  If this times out, then the card is presumably not physically
            //present.

            FILEIO_SD_SPIInitialize_Slow(config);

            //Send CMD0 to reset the media
            //If the card is physically present, then we should get a valid response.
            timeout = 4;
            do
            {
                //Toggle chip select, to make media abandon whatever it may have been doing
                //before.  This ensures the CMD0 is sent freshly after CS is asserted low,
                //minimizing risk of SPI clock pulse master/slave synchronization problems,
                //due to possible application noise on the SCK line.
                (*config->csFunc)(1);       //De-select card
                FILEIO_SD_SPI_Put_Slow(config->index, 0xFF);   //Send some "extraneous" clock pulses.  If a previous
                                      //command was terminated before it completed normally,
                                      //the card might not have received the required clocking
                                      //following the transfer.
                (*config->csFunc)(0);       //Select the card
                timeout--;

                //Send CMD0 to software reset the device
                response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_COMMAND_GO_IDLE_STATE, 0);
            } while((response.r1._byte != 0x01) && (timeout != 0));

            //Check if response was invalid (R1 response uint8_t should be = 0x01 after FILEIO_SD_COMMAND_GO_IDLE_STATE)
            if(response.r1._byte != 0x01)
            {
                FILEIO_SD_MediaDeinitialize(config);
                return false;
            }
            else
            {
                //Card is presumably present.  The SDI pin should have a pull up resistor on
                //it, so the odds of SDI "floating" to 0x01 after sending CMD0 is very
                //remote, unless the media is genuinely present.  Therefore, we should
                //try to perform a full card initialization sequence now.
                FILEIO_SD_MediaInitialize(config);    //Can block and take a long time to execute.
                if(mediaInformation.errorCode == MEDIA_NO_ERROR)
                {
                    /* if the card was initialized correctly, it means it is present */
                    return true;
                }
                else
                {
                    FILEIO_SD_MediaDeinitialize(config);
                    return false;
                }

            }
        }//if(gSDMediaState == FILEIO_SD_STATE_NOT_INITIALIZED)
        else
        {
            //The media already been initialized.  However, it is possible that
            //the user could have unplugged the media, in which case it is no longer
            //present.  We should send it a command, to check the status.

            //Make sure the media is free however.  We don't want to sent the SD card
            //a new command if a previous asynchronous operation (ex: a read or write) request
            //is still pending in the background.
            //The media doesn't support processing more than one command simultaneously.
            if(gSDMediaState == FILEIO_SD_STATE_READY_FOR_COMMAND)
            {
                response = FILEIO_SD_SendCmd(config, FILEIO_SD_COMMAND_SEND_STATUS,0x0);
                if((response.r2._uint16_t & 0xEC0C) != 0x0000)
                {
                    //The card didn't respond with the expected result.  This probably
                    //means it is no longer present.  We can try to re-initialized it,
                    //just to be doubly sure.
                    FILEIO_SD_MediaDeinitialize(config);
                    FILEIO_SD_MediaInitialize(config);    //Can block and take a long time to execute.
                    if(mediaInformation.errorCode == MEDIA_NO_ERROR)
                    {
                        /* if the card was initialized correctly, it means it is present */
                        return true;
                    }
                    else
                    {
                        FILEIO_SD_MediaDeinitialize(config);
                        return false;
                    }
                }
                else
                {
                    //The CMD13 response to SEND_STATUS was valid.  This presumably
                    //means the card is present and working normally.
                    return true;
                }
            }
            else
            {
                //The media was already busy from some other operation.  Assume it is
                //still present.  If the user unplugs it during an operation, this will
                //eventually trigger a timeout, causing it to eventually get polled again.
                return true;
            }

        }

        //Should theoretically never execute to here.  All pathways should have
        //already returned with the status.
        //return true;

    #endif  //End of else of #ifndef MEDIA_SOFT_DETECT
}//end MediaDetect

uint16_t FILEIO_SD_SectorSizeRead(FILEIO_SD_DRIVE_CONFIG * config)
{
    return gMediaSectorSize;
}


uint32_t FILEIO_SD_CapacityRead(FILEIO_SD_DRIVE_CONFIG * config)
{
    return (finalLBA);
}


void FILEIO_SD_IOInitialize (FILEIO_SD_DRIVE_CONFIG * config)
{
    (*config->configurePins)();
}


bool FILEIO_SD_MediaDeinitialize(FILEIO_SD_DRIVE_CONFIG * config)
{
    FILEIO_SD_CSSet tmp = config->csFunc;

    // close the spi bus
    DRV_SPI_Deinitialize (config->index);

    // deselect the device
    (*tmp)(1);

    gSDMediaState = FILEIO_SD_STATE_NOT_INITIALIZED;

    return true;
}


/*****************************************************************************
  Function:
    FILEIO_SD_RESPONSE FILEIO_SD_SendCmd (uint8_t cmd, uint32_t address)
  Summary:
    Sends a command packet to the SD card.
  Conditions:
    None.
  Input:
    config - An SD Drive configuration structure pointer
    cmd - An SD command to send
    address - An address parameter
  Return Values:
    FILEIO_SD_RESPONSE    - The response from the card
                    - Bit 0 - Idle state
                    - Bit 1 - Erase Reset
                    - Bit 2 - Illegal Command
                    - Bit 3 - Command CRC Error
                    - Bit 4 - Erase Sequence Error
                    - Bit 5 - Address Error
                    - Bit 6 - Parameter Error
                    - Bit 7 - Unused. Always 0.
  Side Effects:
    None.
  Description:
    FILEIO_SD_SendCmd prepares a command packet and sends it out over the SPI interface.
    Response data of type 'R1' (as indicated by the SD/MMC product manual is returned.
  Remarks:
    None.
  *****************************************************************************/

FILEIO_SD_RESPONSE FILEIO_SD_SendCmd (FILEIO_SD_DRIVE_CONFIG * config, uint8_t cmd, uint32_t address)
{
    FILEIO_SD_RESPONSE    response;
    FILEIO_SD_CMD_PACKET  CmdPacket;
    uint16_t timeout;
    uint32_t longTimeout;
    
    (*config->csFunc)(0);       // Select card
    
    // Copy over data
    CmdPacket.cmd        = sdmmc_cmdtable[cmd].CmdCode;
    CmdPacket.address    = address;
    CmdPacket.crc        = sdmmc_cmdtable[cmd].CRC;       // Calc CRC here
    
    CmdPacket.TRANSMIT_BIT = 1;             //Set Transmission bit
    
    DRV_SPI_Put(config->index, CmdPacket.cmd);                //Send Command
    DRV_SPI_Put(config->index, CmdPacket.addr3);              //Most Significant uint8_t
    DRV_SPI_Put(config->index, CmdPacket.addr2);
    DRV_SPI_Put(config->index, CmdPacket.addr1);
    DRV_SPI_Put(config->index, CmdPacket.addr0);              //Least Significant uint8_t
    DRV_SPI_Put(config->index, CmdPacket.crc);                //Send CRC

    //Special handling for CMD12 (STOP_TRANSMISSION).  The very first uint8_t after
    //sending the command packet may contain bogus non-0xFF data.  This 
    //"residual data" uint8_t should not be interpreted as the R1 response uint8_t.
    if(cmd == FILEIO_SD_STOP_TRANSMISSION)
    {
        DRV_SPI_Get(config->index); //Perform dummy read to fetch the residual non R1 uint8_t
    } 

    //Loop until we get a response from the media.  Delay (NCR) could be up 
    //to 8 SPI uint8_t times.  First uint8_t of response is always the equivalent of 
    //the R1 uint8_t, even for R1b, R2, R3, R7 responses.
    timeout = FILEIO_SD_NCR_TIMEOUT;
    do
    {
        response.r1._byte = DRV_SPI_Get(config->index);
        timeout--;
    }while((response.r1._byte == FILEIO_SD_FLOATING_BUS_TOKEN) && (timeout != 0));
    
    //Check if we should read more uint8_ts, depending upon the response type expected.  
    if(sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R2)
    {
        response.r2._byte1 = response.r1._byte; //We already received the first uint8_t, just make sure it is in the correct location in the struct.
        response.r2._byte0 = DRV_SPI_Get(config->index); //Fetch the second uint8_t of the response.
    }
    else if(sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R1b)
    {
        //Keep trying to read from the media, until it signals it is no longer
        //busy.  It will continuously send 0x00 uint8_ts until it is not busy.
        //A non-zero value means it is ready for the next command.
        //The R1b response is received after a CMD12 STOP_TRANSMISSION
        //command, where the media card may be busy writing its internal buffer
        //to the flash memory.  This can typically take a few milliseconds, 
        //with a recommended maximum timeout of 250ms or longer for SD cards.
        longTimeout = FILEIO_SD_WRITE_TIMEOUT;
        do
        {
            response.r1._byte = DRV_SPI_Get (config->index);
            longTimeout--;
        }while((response.r1._byte == 0x00) && (longTimeout != 0));

        response.r1._byte = 0x00;
    }
    else if (sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R7) //also used for response R3 type
    {
        //Fetch the other four uint8_ts of the R3 or R7 response.
        //Note: The SD card argument response field is 32-bit, big endian format.
        //However, the C compiler stores 32-bit values in little endian in RAM.
        //When writing to the _returnVal/argument uint8_ts, make sure the order it 
        //gets stored in is correct.      
        response.r7.bytewise.argument._byte3 = DRV_SPI_Get(config->index);
        response.r7.bytewise.argument._byte2 = DRV_SPI_Get(config->index);
        response.r7.bytewise.argument._byte1 = DRV_SPI_Get(config->index);
        response.r7.bytewise.argument._byte0 = DRV_SPI_Get(config->index);
    }

    DRV_SPI_Put(config->index, 0xFF);    //Device requires at least 8 clock pulses after
                             //the response has been sent, before if can process
                             //the next command.  CS may be high or low.

    // see if we are expecting more data or not
    if(!(sdmmc_cmdtable[cmd].moredataexpected))
    {
        (*config->csFunc)(1);
    }

    return(response);
}

#ifdef __XC8
/*****************************************************************************
  Function:
    FILEIO_SD_RESPONSE FILEIO_SD_SendCmdSlow (uint8_t cmd, uint32_t address)
  Summary:
    Sends a command packet to the SD card with bit-bang SPI.
  Conditions:
    None.
  Input:
    Need input cmd index into a rom table of implemented commands.
    Also needs 4 uint8_ts of data as address for some commands (also used for
    other purposes in other commands).
  Return Values:
    Assuming an "R1" type of response, each bit will be set depending upon status:
    FILEIO_SD_RESPONSE    - The response from the card
                    - Bit 0 - Idle state
                    - Bit 1 - Erase Reset
                    - Bit 2 - Illegal Command
                    - Bit 3 - Command CRC Error
                    - Bit 4 - Erase Sequence Error
                    - Bit 5 - Address Error
                    - Bit 6 - Parameter Error
                    - Bit 7 - Unused. Always 0.
    Other response types (ex: R3/R7) have up to 5 uint8_ts of response.  The first
    uint8_t is always the same as the R1 response.  The contents of the other uint8_ts 
    depends on the command.
  Side Effects:
    None.
  Description:
    FILEIO_SD_SendCmd prepares a command packet and sends it out over the SPI interface.
    Response data of type 'R1' (as indicated by the SD/MMC product manual is returned.
    This function is intended to be used when the clock speed of a PIC18 device is
    so high that the maximum SPI divider can't reduce the clock below the maximum
    SD card initialization sequence speed.
  Remarks:
    None.
  ***************************************************************************************/
FILEIO_SD_RESPONSE FILEIO_SD_SendCmdSlow(FILEIO_SD_DRIVE_CONFIG * config, uint8_t cmd, uint32_t address)
{
    FILEIO_SD_RESPONSE    response;
    FILEIO_SD_CMD_PACKET  CmdPacket;
    uint16_t timeout;
    
    (*config->csFunc)(0);                       //Select card
    
    // Copy over data
    CmdPacket.cmd        = sdmmc_cmdtable[cmd].CmdCode;
    CmdPacket.address    = address;
    CmdPacket.crc        = sdmmc_cmdtable[cmd].CRC;       // Calc CRC here
    
    CmdPacket.TRANSMIT_BIT = 1;             //Set Transmission bit
    
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.cmd);                //Send Command
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.addr3);              //Most Significant uint8_t
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.addr2);
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.addr1);
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.addr0);              //Least Significant uint8_t
    FILEIO_SD_SPI_Put_Slow(config->index, CmdPacket.crc);                //Send CRC

    //Special handling for CMD12 (FILEIO_SD_STOP_TRANSMISSION).  The very first byte after
    //sending the command packet may contain bogus non-0xFF data.  This 
    //"residual data" byte should not be interpreted as the R1 response byte.
    if(cmd == FILEIO_SD_STOP_TRANSMISSION)
    {
        FILEIO_SD_SPI_Get_Slow(config->index); //Perform dummy read to fetch the residual non R1 byte
    } 

    //Loop until we get a response from the media.  Delay (NCR) could be up 
    //to 8 SPI byte times.  First byte of response is always the equivalent of
    //the R1 byte, even for R1b, R2, R3, R7 responses.
    timeout = FILEIO_SD_NCR_TIMEOUT;
    do
    {
        response.r1._byte = FILEIO_SD_SPI_Get_Slow(config->index);
        timeout--;
    }while((response.r1._byte == FILEIO_SD_FLOATING_BUS_TOKEN) && (timeout != 0));
    
    
    //Check if we should read more bytes, depending upon the response type expected.
    if(sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R2)
    {
        response.r2._byte1 = response.r1._byte; //We already received the first byte, just make sure it is in the correct location in the struct.
        response.r2._byte0 = FILEIO_SD_SPI_Get_Slow(config->index); //Fetch the second byte of the response.
    }
    else if(sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R1b)
    {
        //Keep trying to read from the media, until it signals it is no longer
        //busy.  It will continuously send 0x00 bytes until it is not busy.
        //A non-zero value means it is ready for the next command.
        timeout = 0xFFFF;
        do
        {
            response.r1._byte = FILEIO_SD_SPI_Get_Slow(config->index);
            timeout--;
        }while((response.r1._byte == 0x00) && (timeout != 0));

        response.r1._byte = 0x00;
    }
    else if (sdmmc_cmdtable[cmd].responsetype == FILEIO_SD_RESPONSE_R7) //also used for response R3 type
    {
        //Fetch the other four uint8_ts of the R3 or R7 response.
        //Note: The SD card argument response field is 32-bit, big endian format.
        //However, the C compiler stores 32-bit values in little endian in RAM.
        //When writing to the _returnVal/argument bytes, make sure the order it
        //gets stored in is correct.      
        response.r7.bytewise.argument._byte3 = FILEIO_SD_SPI_Get_Slow(config->index);
        response.r7.bytewise.argument._byte2 = FILEIO_SD_SPI_Get_Slow(config->index);
        response.r7.bytewise.argument._byte1 = FILEIO_SD_SPI_Get_Slow(config->index);
        response.r7.bytewise.argument._byte0 = FILEIO_SD_SPI_Get_Slow(config->index);
    }

    FILEIO_SD_SPI_Put_Slow(config->index, 0xFF);    //Device requires at least 8 clock pulses after
                             //the response has been sent, before if can process
                             //the next command.  CS may be high or low.

    // see if we are expecting more data or not
    if(!(sdmmc_cmdtable[cmd].moredataexpected))
    {
        (*config->csFunc)(1);
    }

    return(response);
}
#endif


bool FILEIO_SD_SectorRead(FILEIO_SD_DRIVE_CONFIG * config, uint32_t sectorAddress, uint8_t* buffer)
{
    FILEIO_SD_ASYNC_IO info;
    uint8_t status;
    
    //Initialize info structure for using the FILEIO_SD_AsyncReadTasks() function.
    info.wNumBytes = 512;
    info.dwBytesRemaining = 512;
    info.pBuffer = buffer;
    info.dwAddress = sectorAddress;
    info.bStateVariable = FILEIO_SD_ASYNC_READ_QUEUED;
    
    //Blocking loop, until the state machine finishes reading the sector,
    //or a timeout or other error occurs.  FILEIO_SD_AsyncReadTasks() will always
    //return either FILEIO_SD_ASYNC_READ_COMPLETE or FILEIO_SD_ASYNC_READ_FAILED eventually 
    //(could take awhile in the case of timeout), so this won't be a totally
    //infinite blocking loop.
    while(1)
    {
        status = FILEIO_SD_AsyncReadTasks(config, &info);
        if(status == FILEIO_SD_ASYNC_READ_COMPLETE)
        {
            return true;
        }
        else if(status == FILEIO_SD_ASYNC_READ_ERROR)
        {
            return false;
        } 
    }       

    //Impossible to get here, but we will return a value anyay to avoid possible 
    //compiler warnings.
    return false;
}    

/*****************************************************************************
  Function:
    uint8_t FILEIO_SD_AsyncReadTasks(FILEIO_SD_ASYNC_IO* info)
  Summary:
    Speed optimized, non-blocking, state machine based read function that reads 
    data packets from the media, and copies them to a user specified RAM buffer.
  Pre-Conditions:
    The FILEIO_SD_ASYNC_IO structure must be initialized correctly, prior to calling
    this function for the first time.  Certain parameters, such as the user
    data buffer pointer (pBuffer) in the FILEIO_SD_ASYNC_IO struct are allowed to be changed
    by the application firmware, in between each call to FILEIO_SD_AsyncReadTasks().
    Additionally, the media and microcontroller SPI module should have already 
    been initialized before using this function.  This function is mutually
    exclusive with the FILEIO_SD_AsyncWriteTasks() function.  Only one operation
    (either one read or one write) is allowed to be in progress at a time, as
    both functions share statically allocated resources and monopolize the SPI bus.
  Input:
    FILEIO_SD_ASYNC_IO* info -        A pointer to a FILEIO_SD_ASYNC_IO structure.  The 
                            structure contains information about how to complete
                            the read operation (ex: number of total bytes to read,
                            where to copy them once read, maximum number of bytes
                            to return for each call to FILEIO_SD_AsyncReadTasks(), etc.).
  Return Values:
    uint8_t - Returns a status uint8_t indicating the current state of the read 
            operation. The possible return values are:
            
            FILEIO_SD_ASYNC_READ_BUSY - Returned when the state machine is busy waiting for
                             a data start token from the media.  The media has a
                             random access time, which can often be quite long
                             (<= ~3ms typ, with maximum of 100ms).  No data
                             has been copied yet in this case, and the application
                             should keep calling FILEIO_SD_AsyncReadTasks() until either
                             an error/timeout occurs, or FILEIO_SD_ASYNC_READ_NEW_PACKET_READY
                             is returned.
            FILEIO_SD_ASYNC_READ_NEW_PACKET_READY -   Returned after a single packet, of
                                            the specified size (in info->numuint8_ts),
                                            is ready to be read from the 
                                            media and copied to the user 
                                            specified data buffer.  Often, after
                                            receiving this return value, the 
                                            application firmware would want to
                                            update the info->pReceiveBuffer pointer
                                            before calling FILEIO_SD_AsyncReadTasks()
                                            again.  This way, the application can
                                            begin fetching the next packet worth
                                            of data, while still using/consuming
                                            the previous packet of data.
            FILEIO_SD_ASYNC_READ_COMPLETE - Returned when all data bytes in the read
                                 operation have been read and returned successfully,
                                 and the media is now ready for the next operation.
            FILEIO_SD_ASYNC_READ_ERROR - Returned when some failure occurs.  This could be
                               either due to a media timeout, or due to some other
                               unknown type of error.  In this case, the 
                               FILEIO_SD_AsyncReadTasks() handler will terminate
                               the read attempt and will try to put the media 
                               back in a default state, ready for a new command.  
                               The application firmware may then retry the read
                               attempt (if desired) by re-initializing the 
                               FILEIO_SD_ASYNC_IO structure and setting the 
                               bStateVariable = FILEIO_SD_ASYNC_READ_QUEUED.

            
  Side Effects:
    Uses the SPI bus and the media.  The media and SPI bus should not be
    used by any other function until the read operation has either completed
    successfully, or returned with the FILEIO_SD_ASYNC_READ_ERROR condition.
  Description:
    Speed optimized, non-blocking, state machine based read function that reads 
    data packets from the media, and copies them to a user specified RAM buffer.
    This function uses the multi-block read command (and stop transmission command) 
    to perform fast reads of data.  The total amount of data that will be returned 
    on any given call to FILEIO_SD_AsyncReadTasks() will be the info->numuint8_ts parameter.
    However, if the function is called repeatedly, with info->uint8_tsRemaining set
    to a large number, this function can successfully fetch data sizes >> than
    the block size (theoretically anything up to ~4GB, since bytesRemaining is
    a 32-bit uint32_t).  The application firmware should continue calling 
    FILEIO_SD_AsyncReadTasks(), until the FILEIO_SD_ASYNC_READ_COMPLETE value is returned 
    (or FILEIO_SD_ASYNC_READ_ERROR), even if it has already received all of the data expected.
    This is necessary, so the state machine can issue the CMD12 (STOP_TRANMISSION) 
    to terminate the multi-block read operation, once the total expected number 
    of bytes have been read.  This puts the media back into the default state
    ready for a new command.
    
    During normal/successful operations, calls to FILEIO_SD_AsyncReadTasks() 
    would typically return:
    1. FILEIO_SD_ASYNC_READ_BUSY - repeatedly up to several milliseconds, then 
    2. FILEIO_SD_ASYNC_READ_NEW_PACKET_READY - repeatedly, until 512 bytes [media read
        block size] is received, then 
    3. Back to FILEIO_SD_ASYNC_READ_BUSY (for awhile, may be short), then
    4. Back to FILEIO_SD_ASYNC_READ_NEW_PACKET_READY (repeatedly, until the next 512 byte
       boundary, then back to #3, etc.
    5. After all data is received successfully, then the function will return 
       FILEIO_SD_ASYNC_READ_COMPLETE, for all subsequent calls (until a new read operation
       is started, by re-initializing the FILEIO_SD_ASYNC_IO structure, and re-calling
       the function).
    
  Remarks:
    This function will monopolize the SPI module during the operation.  Do not
    use the SPI module for any other purpose, while a fetch operation is in
    progress.  Additionally, the FILEIO_SD_ASYNC_IO structure must not be modified
    in a different context, while the FILEIO_SD_AsyncReadTasks() function is executing.
    In between calls to FILEIO_SD_AsyncReadTasks(), certain parameters, namely the
    info->numbytes and info->pReceiveBuffer are allowed to change however.
    
    The bytesRemaining value must always be an exact integer multiple of numuint8_ts
    for the function to operate correctly.  Additionally, it is recommended, although
    not essential, for the bytesRemaining to be an integer multiple of the media
    read block size.
    
    When starting a read operation, the info->stateVariable must be initialized to
    FILEIO_SD_ASYNC_READ_QUEUED.  All other fields in the info structure should also be
    initialized correctly.
    
    The info->wNumBytes must always be less than or equal to the media block size,
    (which is 512 bytes).  Additionally, info->wNumBytes must always be an exact
    integer factor of the media block size (unless info->dwBytesRemaining is less
    than the media block size).  Example values that are allowed for info->wNumBytes
    are: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512.
  *****************************************************************************/

uint8_t FILEIO_SD_AsyncReadTasks(FILEIO_SD_DRIVE_CONFIG * config, FILEIO_SD_ASYNC_IO* info)
{
    uint8_t bData;
    FILEIO_SD_RESPONSE response;
    static uint16_t blockCounter;
    static uint32_t longTimeoutCounter;
    static bool SingleBlockRead;

    //Check what state we are in, to decide what to do.
    switch(info->bStateVariable)
    {
        case FILEIO_SD_ASYNC_READ_COMPLETE:
            return FILEIO_SD_ASYNC_READ_COMPLETE;
        case FILEIO_SD_ASYNC_READ_QUEUED:
            //Start the read request.

            //Initialize some variables we will use later.
            gSDMediaState = FILEIO_SD_STATE_BUSY;       //Set global media status to busy, so no other code tries to send commands to the SD card
            blockCounter =  FILEIO_SD_MEDIA_BLOCK_SIZE; //Counter will be used later for block boundary tracking
            ioInfo = *info; //Get local copy of structure, for quicker access with less code size

            //SDHC cards are addressed on a 512 byte block basis.  This is 1:1 equivalent
            //to LBA addressing.  For standard capacity media, the media is expecting
            //a complete uint8_t address.  Therefore, to convert from the LBA address to the
            //uint8_t address, we need to multiply by 512.
            if (gSDMode == FILEIO_SD_MODE_NORMAL)
            {
                ioInfo.dwAddress <<= 9; //Equivalent to multiply by 512
            }  
            if(ioInfo.dwBytesRemaining <=  FILEIO_SD_MEDIA_BLOCK_SIZE)
            {
                SingleBlockRead = true;
                response = FILEIO_SD_SendCmd(config, FILEIO_SD_READ_SINGLE_BLOCK, ioInfo.dwAddress);
            }    
            else
            {
                SingleBlockRead = false;
                response = FILEIO_SD_SendCmd(config, FILEIO_SD_READ_MULTI_BLOCK, ioInfo.dwAddress);
            }    
            //Note: SendMMCmd() sends 8 SPI clock cycles after getting the
            //response.  This meets the NAC min timing parameter, so we don't
            //need additional clocking here.
            
            // Make sure the command was accepted successfully
            if(response.r1._byte != 0x00)
            {
                //Perhaps the card isn't initialized or present.
                info->bStateVariable = FILEIO_SD_ASYNC_READ_ABORT;
                return FILEIO_SD_ASYNC_READ_BUSY; 
            }
            
            //We successfully sent the READ_MULTI_BLOCK command to the media.
            //We now need to keep polling the media until it sends us the data
            //start token uint8_t.
            longTimeoutCounter = FILEIO_SD_NAC_TIMEOUT; //prepare timeout counter for next state
            info->bStateVariable = FILEIO_SD_ASYNC_READ_WAIT_START_TOKEN;
            return FILEIO_SD_ASYNC_READ_BUSY;
        case FILEIO_SD_ASYNC_READ_WAIT_START_TOKEN:
            //In this case, we have already issued the READ_MULTI_BLOCK command
            //to the media, and we need to keep polling the media until it sends
            //us the data start token byte.  This could typically take a
            //couple/few milliseconds, up to a maximum of 100ms.
            if(longTimeoutCounter != 0x00000000)
            {
                longTimeoutCounter--;
                bData = DRV_SPI_Get(config->index);
                
                if(bData != FILEIO_SD_FLOATING_BUS_TOKEN)
                {
                    if(bData == FILEIO_SD_DATA_START_TOKEN)
                    {   
                        //We got the start token.  Ready to receive the data
                        //block now.
                        info->bStateVariable = FILEIO_SD_ASYNC_READ_NEW_PACKET_READY;
                        return FILEIO_SD_ASYNC_READ_NEW_PACKET_READY;
                    }
                    else
                    {
                        //We got an unexpected non-0xFF, non-start token uint8_t back?
                        //Some kind of error must have occurred. 
                        info->bStateVariable = FILEIO_SD_ASYNC_READ_ABORT; 
                        return FILEIO_SD_ASYNC_READ_BUSY;
                    }        
                }
                else
                {
                    //Media is still busy.  Start token not received yet.
                    return FILEIO_SD_ASYNC_READ_BUSY;
                }                    
            } 
            else
            {
                //The media didn't respond with the start data token in the timeout
                //interval allowed.  Operation failed.  Abort the operation.
                info->bStateVariable = FILEIO_SD_ASYNC_READ_ABORT; 
                return FILEIO_SD_ASYNC_READ_BUSY;                
            }       
            //Should never execute to here
            
        case FILEIO_SD_ASYNC_READ_NEW_PACKET_READY:
            //We have sent the READ_MULTI_BLOCK command and have successfully
            //received the data start token uint8_t.  Therefore, we are ready
            //to receive raw data uint8_ts from the media.
            if(ioInfo.dwBytesRemaining != 0x00000000)
            {
                //Re-update local copy of pointer and number of uint8_ts to read in this
                //call.  These parameters are allowed to change between packets.
                ioInfo.wNumBytes = info->wNumBytes;
           	    ioInfo.pBuffer = info->pBuffer;
           	    
           	    //Update counters for state tracking and loop exit condition tracking.
                ioInfo.dwBytesRemaining -= ioInfo.wNumBytes;
                blockCounter -= ioInfo.wNumBytes;

                //Now read a ioInfo.wNumuint8_ts packet worth of SPI uint8_ts, 
                //and place the received uint8_ts in the user specified pBuffer.
                //This operation directly dictates data thoroughput in the 
                //application, therefore optimized code should be used for each 
                //processor type.

                DRV_SPI_GetBuffer (config->index, ioInfo.pBuffer, ioInfo.wNumBytes);

                //Check if we have received a multiple of the media block 
                //size (ex: 512 uint8_ts).  If so, the next two uint8_ts are going to 
                //be CRC values, rather than data uint8_ts.  
                if(blockCounter == 0)
                {
                    //Read two uint8_ts to receive the CRC-16 value on the data block.
                    DRV_SPI_Get(config->index);
                    DRV_SPI_Get(config->index);
                    //Following sending of the CRC-16 value, the media may still
                    //need more access time to internally fetch the next block.
                    //Therefore, it will send back 0xFF idle value, until it is
                    //ready.  Then it will send a new data start token, followed
                    //by the next block of useful data.
                    if(ioInfo.dwBytesRemaining != 0x00000000)
                    {
                        info->bStateVariable = FILEIO_SD_ASYNC_READ_WAIT_START_TOKEN;
                    }
                    blockCounter =  FILEIO_SD_MEDIA_BLOCK_SIZE;
                    return FILEIO_SD_ASYNC_READ_BUSY;
                }
                    
                return FILEIO_SD_ASYNC_READ_NEW_PACKET_READY;
            }//if(ioInfo.dwuint8_tsRemaining != 0x00000000)
            else
            {
                //We completed the read operation successfully and have returned
                //all data bytes requested.
                //Send CMD12 to let the media know we are finished reading
                //blocks from it, if we sent a multi-block read request earlier.
                if(SingleBlockRead == false)
                {
                    FILEIO_SD_SendCmd(config, FILEIO_SD_STOP_TRANSMISSION, 0x00000000);
                }
                (*config->csFunc) (1);      // De-select media
                FILEIO_SD_Send8ClockCycles(config->index);
                info->bStateVariable = FILEIO_SD_ASYNC_READ_COMPLETE;
                gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;       //Free the media for new commands, since we are now done with it
                return FILEIO_SD_ASYNC_READ_COMPLETE;
            }
        case FILEIO_SD_ASYNC_READ_ABORT:
            //If the application firmware wants to cancel a read request.
            info->bStateVariable = FILEIO_SD_ASYNC_READ_ERROR;
            //Send CMD12 to terminate the multi-block read request.
            response = FILEIO_SD_SendCmd(config, FILEIO_SD_STOP_TRANSMISSION, 0x00000000);
            //Fall through to FILEIO_SD_ASYNC_READ_ERROR/default case.
        case FILEIO_SD_ASYNC_READ_ERROR:
        default:
            //Some error must have happened.
            (*config->csFunc)(1);       // De-select media
            FILEIO_SD_Send8ClockCycles(config->index);
            gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;
            return FILEIO_SD_ASYNC_READ_ERROR; 
    }//switch(info->stateVariable)    

    #if !defined (__XC8__)
        //Should never get to here.  All pathways should have already returned.
        return FILEIO_SD_ASYNC_READ_ERROR;
    #endif
}    

/*****************************************************************************
  Function:
    uint8_t FILEIO_SD_AsyncWriteTasks(FILEIO_SD_ASYNC_IO* info)
  Summary:
    Speed optimized, non-blocking, state machine based write function that writes
    data from the user specified buffer, onto the media, at the specified 
    media block address.
  Pre-Conditions:
    The FILEIO_SD_ASYNC_IO structure must be initialized correctly, prior to calling
    this function for the first time.  Certain parameters, such as the user
    data buffer pointer (pBuffer) in the FILEIO_SD_ASYNC_IO struct are allowed to be changed
    by the application firmware, in between each call to FILEIO_SD_AsyncWriteTasks().
    Additionally, the media and microcontroller SPI module should have already 
    been initialized before using this function.  This function is mutually
    exclusive with the FILEIO_SD_AsyncReadTasks() function.  Only one operation
    (either one read or one write) is allowed to be in progress at a time, as
    both functions share statically allocated resources and monopolize the SPI bus.
  Input:
    FILEIO_SD_ASYNC_IO* info -        A pointer to a FILEIO_SD_ASYNC_IO structure.  The 
                            structure contains information about how to complete
                            the write operation (ex: number of total uint8_ts to write,
                            where to obtain the uint8_ts from, number of uint8_ts
                            to write for each call to FILEIO_SD_AsyncWriteTasks(), etc.).
  Return Values:
    uint8_t - Returns a status uint8_t indicating the current state of the write 
            operation. The possible return values are:
            
            FILEIO_SD_ASYNC_WRITE_BUSY - Returned when the state machine is busy waiting for
                             the media to become ready to accept new data.  The 
                             media has write time, which can often be quite long
                             (a few ms typ, with maximum of 250ms).  The application
                             should keep calling FILEIO_SD_AsyncWriteTasks() until either
                             an error/timeout occurs, FILEIO_SD_ASYNC_WRITE_SEND_PACKET
                             is returned, or FILEIO_SD_ASYNC_WRITE_COMPLETE is returned.
            FILEIO_SD_ASYNC_WRITE_SEND_PACKET -   Returned when the FILEIO_SD_AsyncWriteTasks()
                                        handler is ready to consume data and send
                                        it to the media.  After FILEIO_SD_ASYNC_WRITE_SEND_PACKET
                                        is returned, the application should make certain
                                        that the info->wNumuint8_ts and pBuffer parameters
                                        are correct, prior to calling 
                                        FILEIO_SD_AsyncWriteTasks() again.  After
                                        the function returns, the application is
                                        then free to write new data into the pBuffer
                                        RAM location. 
            FILEIO_SD_ASYNC_WRITE_COMPLETE - Returned when all data uint8_ts in the write
                                 operation have been written to the media successfully,
                                 and the media is now ready for the next operation.
            FILEIO_SD_ASYNC_WRITE_ERROR - Returned when some failure occurs.  This could be
                               either due to a media timeout, or due to some other
                               unknown type of error.  In this case, the 
                               FILEIO_SD_AsyncWriteTasks() handler will terminate
                               the write attempt and will try to put the media 
                               back in a default state, ready for a new command.  
                               The application firmware may then retry the write
                               attempt (if desired) by re-initializing the 
                               FILEIO_SD_ASYNC_IO structure and setting the 
                               bStateVariable = FILEIO_SD_ASYNC_WRITE_QUEUED.

            
  Side Effects:
    Uses the SPI bus and the media.  The media and SPI bus should not be
    used by any other function until the read operation has either completed
    successfully, or returned with the FILEIO_SD_ASYNC_WRITE_ERROR condition.
  Description:
    Speed optimized, non-blocking, state machine based write function that writes 
    data packets to the media, from a user specified RAM buffer.
    This function uses either the single block or multi-block write command 
    to perform fast writes of the data.  The total amount of data that will be 
    written on any given call to FILEIO_SD_AsyncWriteTasks() will be the 
    info->numuint8_ts parameter.
    However, if the function is called repeatedly, with info->dwuint8_tsRemaining
    set to a large number, this function can successfully write data sizes >> than
    the block size (theoretically anything up to ~4GB, since dwuint8_tsRemaining is 
    a 32-bit uint32_t).  The application firmware should continue calling 
    FILEIO_SD_AsyncWriteTasks(), until the FILEIO_SD_ASYNC_WRITE_COMPLETE value is returned 
    (or FILEIO_SD_ASYNC_WRITE_ERROR), even if it has already sent all of the data expected.
    This is necessary, so the state machine can finish the write process and 
    terminate the multi-block write operation, once the total expected number 
    of uint8_ts have been written.  This puts the media back into the default state 
    ready for a new command.
    
    During normal/successful operations, calls to FILEIO_SD_AsyncWriteTasks() 
    would typically return:
    1. FILEIO_SD_ASYNC_WRITE_SEND_PACKET - repeatedly, until 512 uint8_ts [media read 
        block size] is received, then 
    2. FILEIO_SD_ASYNC_WRITE_BUSY (for awhile, could be a long time, many milliseconds), then
    3. Back to FILEIO_SD_ASYNC_WRITE_SEND_PACKET (repeatedly, until the next 512 uint8_t
       boundary, then back to #2, etc.
    4. After all data is copied successfully, then the function will return 
       FILEIO_SD_ASYNC_WRITE_COMPLETE, for all subsequent calls (until a new write operation
       is started, by re-initializing the FILEIO_SD_ASYNC_IO structure, and re-calling
       the function).
    
  Remarks:
    When starting a read operation, the info->stateVariable must be initialized to
    FILEIO_SD_ASYNC_WRITE_QUEUED.  All other fields in the info structure should also be
    initialized correctly.

    This function will monopolize the SPI module during the operation.  Do not
    use the SPI module for any other purpose, while a write operation is in
    progress.  Additionally, the FILEIO_SD_ASYNC_IO structure must not be modified
    in a different context, while the FILEIO_SD_AsyncReadTasks() function is 
    actively executing.
    In between calls to FILEIO_SD_AsyncWriteTasks(), certain parameters, namely the
    info->wNumuint8_ts and info->pBuffer are allowed to change however.
    
    The dwuint8_tsRemaining value must always be an exact integer multiple of wNumuint8_ts 
    for the function to operate correctly.  Additionally, it is required that
    the wNumuint8_ts parameter, must always be less than or equal to the media block size,
    (which is 512 uint8_ts).  Additionally, info->wNumuint8_ts must always be an exact 
    integer factor of the media block size.  Example values that are allowed for
    info->wNumuint8_ts are: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512.
  *****************************************************************************/
uint8_t FILEIO_SD_AsyncWriteTasks(FILEIO_SD_DRIVE_CONFIG * config, FILEIO_SD_ASYNC_IO* info)
{
    static uint8_t data_byte;
    static uint16_t blockCounter;
    static uint32_t WriteTimeout;
    static uint8_t command;
    uint32_t preEraseBlockCount;
    FILEIO_SD_RESPONSE response;
    
    //Check what state we are in, to decide what to do.
    switch(info->bStateVariable)
    {
        case FILEIO_SD_ASYNC_WRITE_COMPLETE:
            return FILEIO_SD_ASYNC_WRITE_COMPLETE;
        case FILEIO_SD_ASYNC_WRITE_QUEUED:
            //Initiate the write sequence.
            gSDMediaState = FILEIO_SD_STATE_BUSY;         //Let other code in the app know that the media is busy (so it doesn't also try to send the SD card commands of it's own)
            blockCounter = FILEIO_SD_MEDIA_BLOCK_SIZE;    //Initialize counter.  Will be used later for block boundary tracking.

            //Copy input structure into a statically allocated global instance 
            //of the structure, for faster local access of the parameters with 
            //smaller code size.
            ioInfo = *info;

            //Check if we are writing only a single block worth of data, or 
            //multiple blocks worth of data.
            if(ioInfo.dwBytesRemaining <=  FILEIO_SD_MEDIA_BLOCK_SIZE)
            {
                command = FILEIO_SD_WRITE_SINGLE_BLOCK;
            }    
            else
            {
                command = FILEIO_SD_WRITE_MULTI_BLOCK;
                
                //Compute the number of blocks that we are going to be writing in this multi-block write operation.
                preEraseBlockCount = (ioInfo.dwBytesRemaining >> 9); //Divide by 512 to get the number of blocks to write
                //Always need to erase at least one block.
                if(preEraseBlockCount == 0)
                {
                    preEraseBlockCount++;   
                } 
    
                //Should send CMD55/ACMD23 to let the media know how many blocks it should 
                //pre-erase.  This isn't essential, but it allows for faster multi-block 
                //writes, and probably also reduces flash wear on the media.
                response = FILEIO_SD_SendCmd(config, FILEIO_SD_APP_CMD, 0x00000000);    //Send CMD55
                if(response.r1._byte == 0x00)   //Check if successful.
                {
                    FILEIO_SD_SendCmd(config, FILEIO_SD_SET_WRITE_BLOCK_ERASE_COUNT , preEraseBlockCount);    //Send ACMD23
                }
            }    

            //The info->dwAddress parameter is the block address.
            //For standard capacity SD cards, the card expects a complete uint8_t address.
            //To convert the block address into a uint8_t address, we multiply by the block size (512).
            //For SDHC (high capacity) cards, the card expects a block address already, so no
            //address conversion is needed
            if (gSDMode == FILEIO_SD_MODE_NORMAL)  
            {
                ioInfo.dwAddress <<= 9;   //<< 9 multiplies by 512
            }    

            //Send the write single or write multi command, with the LBA or uint8_t 
            //address (depending upon SDHC or standard capacity card)
            response = FILEIO_SD_SendCmd(config, command, ioInfo.dwAddress);

            //See if it was accepted
            if(response.r1._byte != 0x00)
            {
                //Perhaps the card isn't initialized or present.
                info->bStateVariable = FILEIO_SD_ASYNC_WRITE_ERROR;
                return FILEIO_SD_ASYNC_WRITE_ERROR; 
            }    
            else
            {
                //Card is ready to receive start token and data uint8_ts.
                info->bStateVariable = FILEIO_SD_ASYNC_WRITE_TRANSMIT_PACKET;
            } 
            return FILEIO_SD_ASYNC_WRITE_SEND_PACKET;   

        case FILEIO_SD_ASYNC_WRITE_TRANSMIT_PACKET:
            //Check if we just finished programming a block, or we are starting
            //for the first time.  In this case, we need to send the data start token.
            if(blockCounter ==  FILEIO_SD_MEDIA_BLOCK_SIZE)
            {
                //Send the correct data start token, based on the type of write we are doing
                if(command == FILEIO_SD_WRITE_MULTI_BLOCK)
                {
                    DRV_SPI_Put (config->index, FILEIO_SD_DATA_START_MULTI_BLOCK_TOKEN);
                }
                else
                {
                    //Else must be a single block write
                    DRV_SPI_Put (config->index, FILEIO_SD_DATA_START_TOKEN);
                }        
            } 
               
            //Update local copy of pointer and uint8_t count.  Application firmware
            //is allowed to change these between calls to this handler function.
            ioInfo.wNumBytes = info->wNumBytes;
            ioInfo.pBuffer = info->pBuffer;
            
            //Keep track of variables for loop/state exit conditions.
            ioInfo.dwBytesRemaining -= ioInfo.wNumBytes;
            blockCounter -= ioInfo.wNumBytes;
            
            //Now send a packet of raw data uint8_ts to the media, over SPI.
            //This code directly impacts data throughput in a significant way.  
            //Special care should be used to make sure this code is speed optimized.
            DRV_SPI_PutBuffer (config->index, ioInfo.pBuffer, ioInfo.wNumBytes);
 
            //Check if we have finished sending a 512 uint8_t block.  If so,
            //need to receive 16-bit CRC, and retrieve the data_response token
            if(blockCounter == 0)
            {
                blockCounter =  FILEIO_SD_MEDIA_BLOCK_SIZE;    //Re-initialize counter
                
                //Add code to compute CRC, if using CRC. By default, the media 
                //doesn't use CRC unless it is enabled manually during the card 
                //initialization sequence.
                FILEIO_SD_CRCSend(config->index);  //Send 16-bit CRC for the data block just sent
                
                //Read response token uint8_t from media, mask out top three don't 
                //care bits, and check if there was an error
                if((DRV_SPI_Get(config->index) &  FILEIO_SD_WRITE_RESPONSE_TOKEN_MASK) != FILEIO_SD_DATA_ACCEPTED)
                {
                    //Something went wrong.  Try and terminate as gracefully as 
                    //possible, so as allow possible recovery.
                    info->bStateVariable = FILEIO_SD_ASYNC_WRITE_ABORT; 
                    return FILEIO_SD_ASYNC_WRITE_BUSY;
                }
                
                //The media will now send busy token (0x00) uint8_ts until
                //it is internally ready again (after the block is successfully
                //writen and the card is ready to accept a new block.
                info->bStateVariable = FILEIO_SD_ASYNC_WRITE_MEDIA_BUSY;
                WriteTimeout = FILEIO_SD_WRITE_TIMEOUT;       //Initialize timeout counter
                return FILEIO_SD_ASYNC_WRITE_BUSY;
            }//if(blockCounter == 0)
            
            //If we get to here, we haven't reached a block boundary yet.  Keep 
            //on requesting packets of data from the application.
            return FILEIO_SD_ASYNC_WRITE_SEND_PACKET;   

        case FILEIO_SD_ASYNC_WRITE_MEDIA_BUSY:
            if(WriteTimeout != 0)
            {
                WriteTimeout--;
                FILEIO_SD_Send8ClockCycles(config->index);  //Dummy read to gobble up a uint8_t (ex: to ensure we meet NBR timing parameter)
                data_byte = DRV_SPI_Get(config->index);  //Poll the media.  Will return 0x00 if still busy.  Will return non-0x00 is ready for next data block.
                if(data_byte != 0x00)
                {
                    //The media is done and is no longer busy.  Go ahead and
                    //either send the next packet of data to the media, or the stop
                    //token if we are finished.
                    if(ioInfo.dwBytesRemaining == 0)
                    {
                        WriteTimeout = FILEIO_SD_WRITE_TIMEOUT;
                        if(command == FILEIO_SD_WRITE_MULTI_BLOCK)
                        {
                            //We finished sending all uint8_ts of data.  Send the stop token uint8_t.
                            DRV_SPI_Put (config->index, FILEIO_SD_DATA_STOP_TRAN_TOKEN);
                            //After sending the stop transmission token, we need to
                            //gobble up one uint8_t before checking for media busy (0x00).
                            //This is to meet the NBR timing parameter.  During the NBR
                            //interval the SD card may not respond with the busy signal, even
                            //though it is internally busy.
                            FILEIO_SD_Send8ClockCycles(config->index);
                                                
                            //The media still needs to finish internally writing.
                            info->bStateVariable = FILEIO_SD_ASYNC_STOP_TOKEN_SENT_WAIT_BUSY;
                            return FILEIO_SD_ASYNC_WRITE_BUSY;
                        }
                        else
                        {
                            //In this case we were doing a single block write,
                            //so no stop token is necessary.  In this case we are
                            //now fully complete with the write operation.
                            (*config->csFunc)(1);       // De-select media
                            FILEIO_SD_Send8ClockCycles(config->index);
                            info->bStateVariable = FILEIO_SD_ASYNC_WRITE_COMPLETE;
                            gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;       //Free the media for new commands, since we are now done with it
                            return FILEIO_SD_ASYNC_WRITE_COMPLETE;                            
                        }                            
                        
                    }
                    //Else we have more data to write in the multi-block write.    
                    info->bStateVariable = FILEIO_SD_ASYNC_WRITE_TRANSMIT_PACKET;  
                    return FILEIO_SD_ASYNC_WRITE_SEND_PACKET;                    
                }    
                else
                {
                    //The media is still busy.
                    return FILEIO_SD_ASYNC_WRITE_BUSY;
                }    
            }
            else
            {
                //Timeout occurred.  Something went wrong.  The media should not 
                //have taken this long to finish the write.
                info->bStateVariable = FILEIO_SD_ASYNC_WRITE_ABORT;
                return FILEIO_SD_ASYNC_WRITE_BUSY;
            }        
        
        case FILEIO_SD_ASYNC_STOP_TOKEN_SENT_WAIT_BUSY:
            //We already sent the stop transmit token for the multi-block write 
            //operation.  Now all we need to do, is keep waiting until the card
            //signals it is no longer busy.  Card will keep sending 0x00 uint8_ts
            //until it is no longer busy.
            if(WriteTimeout != 0)
            {
                WriteTimeout--;
                data_byte = DRV_SPI_Get (config->index);
                //Check if card is no longer busy.  
                if(data_byte != 0x00)
                {
                    //If we get to here, multi-block write operation is fully
                    //complete now.  

                    //Should send CMD13 (SEND_STATUS) after a programming sequence, 
                    //to confirm if it was successful or not inside the media.
                                
                    //Prepare to receive the next command.
                    (*config->csFunc)(1);       // De-select media
                    FILEIO_SD_Send8ClockCycles(config->index);  //NEC timing parameter clocking
                    info->bStateVariable = FILEIO_SD_ASYNC_WRITE_COMPLETE;
                    gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;       //Free the media for new commands, since we are now done with it
                    return FILEIO_SD_ASYNC_WRITE_COMPLETE;
                }
                //If we get to here, the media is still busy with the write.
                return FILEIO_SD_ASYNC_WRITE_BUSY;    
            }    
            //Timeout occurred.  Something went wrong.  Fall through to FILEIO_SD_ASYNC_WRITE_ABORT.
        case FILEIO_SD_ASYNC_WRITE_ABORT:
            //An error occurred, and we need to stop the write sequence so as to try and allow
            //for recovery/re-attempt later.
            FILEIO_SD_SendCmd(config, FILEIO_SD_STOP_TRANSMISSION, 0x00000000);
            (*config->csFunc)(1);  // De-select media
            FILEIO_SD_Send8ClockCycles(config->index);  //After raising CS pin, media may not tri-state data out for 1 bit time.
            info->bStateVariable = FILEIO_SD_ASYNC_WRITE_ERROR; 
            //Fall through to default case.
        default:
            //Used for FILEIO_SD_ASYNC_WRITE_ERROR case.
            gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;       //Free the media for new commands, since we are now done with it
            return FILEIO_SD_ASYNC_WRITE_ERROR; 
    }//switch(info->stateVariable)    
    
#if !defined (__XC8__)
    //Should never execute to here.  All pathways should have a hit a return already.
    info->bStateVariable = FILEIO_SD_ASYNC_WRITE_ABORT;
    return FILEIO_SD_ASYNC_WRITE_BUSY;
#endif
} 


bool FILEIO_SD_SectorWrite(FILEIO_SD_DRIVE_CONFIG * config, uint32_t sectorAddress, uint8_t* buffer, bool allowWriteToZero)
{
    static FILEIO_SD_ASYNC_IO info;
    uint8_t status;

    if(allowWriteToZero == false)
    {
        if(sectorAddress == 0x00000000)
        {
            return false;
        }    
    }    
    
    //Initialize structure so we write a single sector worth of data.
    info.wNumBytes = 512;
    info.dwBytesRemaining = 512;
    info.pBuffer = buffer;
    info.dwAddress = sectorAddress;
    info.bStateVariable = FILEIO_SD_ASYNC_WRITE_QUEUED;
    
    //Repeatedly call the write handler until the operation is complete (or a
    //failure/timeout occurred).
    while(1)
    {
        status = FILEIO_SD_AsyncWriteTasks(config, &info);
        if(status == FILEIO_SD_ASYNC_WRITE_COMPLETE)
        {
            return true;
        }    
        else if(status == FILEIO_SD_ASYNC_WRITE_ERROR)
        {
            return false;
        }
    }    
    return true;
}    


bool FILEIO_SD_WriteProtectStateGet(FILEIO_SD_DRIVE_CONFIG * config)
{
    return (*config->wpFunc)();
}


/*******************************************************************************
  Function:
    void Delayms (uint8_t milliseconds)
  Summary:
    Delay.
  Conditions:
    None.
  Input:
    uint8_t milliseconds - Number of ms to delay
  Return:
    None.
  Side Effects:
    None.
  Description:
    The Delayms function will delay a specified number of milliseconds.  Used for SPI
    timing.
  Remarks:
    Depending on compiler revisions, this function may not delay for the exact 
    time specified.  This shouldn't create a significant problem.
*******************************************************************************/

void Delayms(uint8_t milliseconds)
{
    uint8_t    ms;
    uint32_t   count;
    
    ms = milliseconds;
    while (ms--)
    {
        count = FILEIO_SD_MILLISECOND_DELAY;
        while (count--);
    }
    Nop();
    return;
}

/*****************************************************************************
  Function:
    void FILEIO_SD_SPISlowInitialize(void)
  Summary:
    Initializes the SPI module to operate at low SPI frequency <= 400kHz.
  Conditions:
    Processor type and SYS_CLK_FrequencyInstructionGet() macro have to be defined correctly
    to get the correct SPI frequency.
  Input:
    config - An SD Drive configuration structure pointer
  Return Values:
    None.  Initializes the hardware SPI module (except on PIC18).  On PIC18,
    The SPI is bit banged to achieve low frequencies, but this function still
    initializes the I/O pins. 
  Side Effects:
    None.
  Description:
    This function initializes and enables the SPI module, configured for low 
    SPI frequency, so as to be compatible with media cards which require <400kHz
    SPI frequency during initialization.
  Remarks:
    None.
  ***************************************************************************************/
void FILEIO_SD_SPISlowInitialize(FILEIO_SD_DRIVE_CONFIG * config)
{
    DRV_SPI_INIT_DATA spiInitData;

    spiInitData.mode = 0;
    spiInitData.spibus_mode = SPI_BUS_MODE_2;

#if defined __XC16__ || defined __XC32__
    	#ifdef __XC32__
            spiInitData.cke = 0;
    	    spiInitData.baudRate = SPICalculateBRG(SYS_CLK_FrequencyPeripheralGet(), 400000);
            spiInitData.channel = config->index;
            //DRV_SPI_Initialize(config->index, &spiInitData);
			DRV_SPI_Initialize(&spiInitData);
//    		OpenSPI(SPI_START_CFG_1, SPI_START_CFG_2);
    	#else //else C30 = PIC24/dsPIC devices
            #if defined(DRV_SPI_CONFIG_V2_ENABLED)
            spiInitData.cke = 0;
            spiInitData.primaryPrescale = (SYS_CLK_FrequencyInstructionGet() / 400000 );
            spiInitData.mode = SPI_TRANSFER_MODE_8BIT;
            #else
            uint16_t spiconvalue = 0x0003;
            uint16_t timeout;

            spiInitData.cke = 0;

            // Calculate the prescaler needed for the clock
    	    timeout = SYS_CLK_FrequencyInstructionGet() / 400000;
    	    // if timeout is less than 400k and greater than 100k use a 1:1 prescaler
    	    if (timeout == 0)
    	    {
                spiInitData.primaryPrescale = PRI_PRESCAL_1_1;
                spiInitData.secondaryPrescale = SEC_PRESCAL_1_1;
    	    }
    	    else
    	    {
    	        while (timeout != 0)
    	        {
    	            if (timeout > 8)
    	            {
    	                spiconvalue--;
    	                // round up
    	                if ((timeout % 4) != 0)
    	                    timeout += 4;
    	                timeout /= 4;
    	            }
    	            else
    	            {
    	                break;
    	            }
    	        }
    	        
    	        timeout--;
    	    
                spiInitData.primaryPrescale = spiconvalue;
                spiInitData.secondaryPrescale = (~timeout) & 0b111;
    	    }
            #endif

            spiInitData.channel = config->index;
            DRV_SPI_Initialize(&spiInitData);
        #endif   //#ifdef __XC32__ (and corresponding #else)
    #else //must be PIC18 device
        spiInitData.cke = 0;
        if (SYS_CLK_FrequencyPeripheralGet() > 25600000)
        {
            return;
        }
        else
        {
            if (SYS_CLK_FrequencyPeripheralGet() >= 6400000)
            {
                spiInitData.divider = 2;                // x64 divider
            }
            else if (SYS_CLK_FrequencyPeripheralGet() >= 1600000)
            {
                spiInitData.divider = 1;                // x16 divider
            }
            else if (SYS_CLK_FrequencyPeripheralGet() > 400000)
            {
                spiInitData.divider = 0;                // x4 divider
            }
            else
            {
                // This will produce a failure
                return;
            }
            spiInitData.channel = config->index;
            DRV_SPI_Initialize (&spiInitData);
        }
    #endif //#if defined __XC16__ || defined __XC32__
}    


FILEIO_MEDIA_INFORMATION *  FILEIO_SD_MediaInitialize (FILEIO_SD_DRIVE_CONFIG * config)
{
    uint16_t timeout;
    FILEIO_SD_RESPONSE response;
	uint8_t CSDResponse[20];
	uint8_t count, index;
	uint32_t c_size;
	uint8_t c_size_mult;
	uint8_t block_len;
    DRV_SPI_INIT_DATA spiInitData;

	#ifdef __DEBUG_UART
	InitUART();
	#endif
 
    //Initialize global variables.  Will get updated later with valid data once
    //the data is known.
    gSDMediaState = FILEIO_SD_STATE_NOT_INITIALIZED;
    mediaInformation.errorCode = MEDIA_NO_ERROR;
    mediaInformation.validityFlags.value = 0;
    finalLBA = 0x00000000;	//Will compute a valid size later, from the CSD register values we get from the card
    gSDMode = FILEIO_SD_MODE_NORMAL;           //Will get updated later with real value, once we know based on initialization flow.

    (*config->csFunc)(1);   //Initialize Chip Select line (1 = card not selected)

    //MMC media powers up in the open-drain mode and cannot handle a clock faster
    //than 400kHz. Initialize SPI port to <= 400kHz
    FILEIO_SD_SPIInitialize_Slow(config);
    
    #ifdef __DEBUG_UART  
    PrintROMASCIIStringUART("\r\n\r\nInitializing Media\r\n"); 
    #endif

    //Media wants the longer of: Vdd ramp time, 1 ms fixed delay, or 74+ clock pulses.
    //According to spec, CS should be high during the 74+ clock pulses.
    //In practice it is preferable to wait much longer than 1ms, in case of
    //contact bounce, or incomplete mechanical insertion (by the time we start
    //accessing the media). 
    Delayms(30);
    (*config->csFunc)(1);
    //Generate 80 clock pulses.
    for(timeout=0; timeout<10u; timeout++)
        FILEIO_SD_SPI_Put_Slow(config->index, 0xFF);


    // Send CMD0 (with CS = 0) to reset the media and put SD cards into SPI mode.
    timeout = 100;
    do
    {
        //Toggle chip select, to make media abandon whatever it may have been doing
        //before.  This ensures the CMD0 is sent freshly after CS is asserted low,
        //minimizing risk of SPI clock pulse master/slave synchronization problems, 
        //due to possible application noise on the SCK line.
        (*config->csFunc)(1);
        FILEIO_SD_SPI_Put_Slow(config->index, 0xFF);  //Send some "extraneous" clock pulses.  If a previous
                                            //command was terminated before it completed normally,
                                            //the card might not have received the required clocking
                                            //following the transfer.
        (*config->csFunc)(0);
        timeout--;

        //Send CMD0 to software reset the device
        response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_GO_IDLE_STATE, 0x0);
    }while((response.r1._byte != 0x01) && (timeout != 0));
    //Check if all attempts failed and we timed out.  Normally, this won't happen,
    //unless maybe the SD card was busy, because it was previously performing a
    //read or write operation, when it was interrupted by the microcontroller getting
    //reset or power cycled, without also resetting or power cycling the SD card.
    //In this case, the SD card may still be busy (ex: trying to respond with the 
    //read request data), and may not be ready to process CMD0.  In this case,
    //we can try to recover by issuing CMD12 (STOP_TRANSMISSION).
    if(timeout == 0)
    {
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("Media failed CMD0 too many times. R1 response uint8_t = ");
        PrintRAMuint8_tsUART(((unsigned char*)&response + 1), 1);
        UARTSendLineFeedCarriageReturn();
        PrintROMASCIIStringUART("Trying CMD12 to recover.\r\n");
        #endif

        (*config->csFunc)(1);
        FILEIO_SD_SPI_Put_Slow(config->index, 0xFF);        //Send some "extraneous" clock pulses.  If a previous
                                                            //command was terminated before it completed normally,
                                                            //the card might not have received the required clocking
                                                            //following the transfer.
        (*config->csFunc)(0);

        //Send CMD12, to stop any read/write transaction that may have been in progress
        response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_STOP_TRANSMISSION, 0x0);    //Blocks until SD card signals non-busy
        //Now retry to send send CMD0 to perform software reset on the media
        response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_GO_IDLE_STATE, 0x0);
        if(response.r1._byte != 0x01) //Check if card in idle state now.
        {
            //Card failed to process CMD0 yet again.  At this point, the proper thing
            //to do would be to power cycle the card and retry, if the host 
            //circuitry supports disconnecting the SD card power.  Since the
            //SD/MMC PICtail+ doesn't support software controlled power removal
            //of the SD card, there is nothing that can be done with this hardware.
            //Therefore, we just give up now.  The user needs to physically 
            //power cycle the media and/or the whole board.
            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("Media still failed CMD0. Cannot initialize card, returning.\r\n");
            #endif   
            mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
            return &mediaInformation;
        }            
        else
        {
            //Card successfully processed CMD0 and is now in the idle state.
            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("Media successfully processed CMD0 after CMD12.\r\n");
            #endif        
        }    
    }//if(timeout == 0) [for the CMD0 transmit loop]
    else
    {
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("Media successfully processed CMD0.\r\n");
        #endif        
    }       
    

    //Send CMD8 (SEND_IF_COND) to specify/request the SD card interface condition (ex: indicate what voltage the host runs at).
    //0x000001AA --> VHS = 0001b = 2.7V to 3.6V.  The 0xAA LSB is the check pattern, and is arbitrary, but 0xAA is recommended (good blend of 0's and '1's).
    //The SD card has to echo back the check pattern correctly however, in the R7 response.
    //If the SD card doesn't support the operating voltage range of the host, then it may not respond.
    //If it does support the range, it will respond with a type R7 response packet (6 uint8_ts/48 bits).	        
    //Additionally, if the SD card is MMC or SD card v1.x spec device, then it may respond with
    //invalid command.  If it is a v2.0 spec SD card, then it is mandatory that the card respond
    //to CMD8.
    response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_SEND_IF_COND, 0x1AA);   //Note: If changing "0x1AA", CRC value in table must also change.
    if(((response.r7.bytewise.argument._returnVal & 0xFFF) == 0x1AA) && (!response.r7.bitwise.bits.ILLEGAL_CMD))
   	{
        //If we get to here, the device supported the CMD8 command and didn't complain about our host
        //voltage range.
        //Most likely this means it is either a v2.0 spec standard or high capacity SD card (SDHC)
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("Media successfully processed CMD8. Response = ");
        PrintRAMuint8_tsUART(((unsigned char*)&response + 1), 4);
        UARTSendLineFeedCarriageReturn();
        #endif

		//Send CMD58 (Read OCR [operating conditions register]).  Response type is R3, which has 5 uint8_ts.
		//uint8_t 4 = normal R1 response uint8_t, uint8_ts 3-0 are = OCR register value.
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("Sending CMD58.\r\n");
        #endif
        response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_READ_OCR, 0x0);
        //Now that we have the OCR register value in the response packet, we could parse
        //the register contents and learn what voltage the SD card wants to run at.
        //If our host circuitry has variable power supply capability, it could 
        //theoretically adjust the SD card Vdd to the minimum of the OCR to save power.
		
		//Now send CMD55/ACMD41 in a loop, until the card is finished with its internal initialization.
		//Note: SD card specs recommend >= 1 second timeout while waiting for ACMD41 to signal non-busy.
		for(timeout = 0; timeout < 0xFFFF; timeout++)
		{				
			//Send CMD55 (lets SD card know that the next command is application specific (going to be ACMD41)).
			FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_APP_CMD, 0x00000000);
			
			//Send ACMD41.  This is to check if the SD card is finished booting up/ready for full frequency and all
			//further commands.  Response is R3 type (6 uint8_ts/48 bits, middle four uint8_ts contain potentially useful data).
            //Note: When sending ACMD41, the HCS bit is bit 30, and must be = 1 to tell SD card the host supports SDHC
			response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_SD_SEND_OP_COND,0x40000000); //bit 30 set
			
			//The R1 response should be = 0x00, meaning the card is now in the "standby" state, instead of
			//the "idle" state (which is the default initialization state after CMD0 reset is issued).  Once
			//in the "standby" state, the SD card is finished with basic initialization and is ready 
			//for read/write and other commands.
			if(response.r1._byte == 0)
			{
    		    #ifdef __DEBUG_UART  
                PrintROMASCIIStringUART("Media successfully processed CMD55/ACMD41 and is no longer busy.\r\n");
				#endif
				break;  //Break out of for() loop.  Card is finished initializing.
            }				
		}		
		if(timeout >= 0xFFFF)
		{
            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("Media Timeout on CMD55/ACMD41.\r\n");
            #endif
    		mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
        }				
		
		
        //Now send CMD58 (Read OCR register).  The OCR register contains important
        //info we will want to know about the card (ex: standard capacity vs. SDHC).
        response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_READ_OCR, 0x0);

		//Now check the CCS bit (OCR bit 30) in the OCR register, which is in our response packet.
		//This will tell us if it is a SD high capacity (SDHC) or standard capacity device.
		if(response.r7.bytewise.argument._returnVal & 0x40000000)    //Note the HCS bit is only valid when the busy bit is also set (indicating device ready).
		{
			gSDMode = FILEIO_SD_MODE_HC;
			
		    #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("Media successfully processed CMD58: SD card is SDHC v2.0 (or later) physical spec type.\r\n");
            #endif
        }				
        else
        {
            gSDMode = FILEIO_SD_MODE_NORMAL;

            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("Media successfully processed CMD58: SD card is standard capacity v2.0 (or later) spec.\r\n");
            #endif
        } 
        //SD Card should now be finished with initialization sequence.  Device should be ready
        //for read/write commands.

	}//if(((response.r7.bytewise._returnVal & 0xFFF) == 0x1AA) && (!response.r7.bitwise.bits.ILLEGAL_CMD))
    else
	{
        //The CMD8 wasn't supported.  This means the card is not a v2.0 card.
        //Presumably the card is v1.x device, standard capacity (not SDHC).

        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("CMD8 Unsupported: Media is most likely MMC or SD 1.x device.\r\n");
        #endif


        (*config->csFunc)(1);                              // deselect the devices
        Delayms(1);
        (*config->csFunc)(0);                              // select the device

        //The CMD8 wasn't supported.  This means the card is definitely not a v2.0 SDHC card.
        gSDMode = FILEIO_SD_MODE_NORMAL;
    	// According to the spec CMD1 must be repeated until the card is fully initialized
    	timeout = 0x1FFF;
        do
        {
            //Send CMD1 to initialize the media.
            response = FILEIO_SD_SendMediaCmd_Slow(config, FILEIO_SD_SEND_OP_COND, 0x00000000);    //When argument is 0x00000000, this queries MMC cards for operating voltage range
            timeout--;
        }while((response.r1._byte != 0x00) && (timeout != 0));
        // see if it failed
        if(timeout == 0)
        {
            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("CMD1 failed.\r\n");
            #endif

            mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
            (*config->csFunc)(1);                              // deselect the devices
        }
        else
        {
            #ifdef __DEBUG_UART  
            PrintROMASCIIStringUART("CMD1 Successfully processed, media is no longer busy.\r\n");
            #endif
            
            //Set read/write block length to 512 bytes.  Note: commented out since
            //this theoretically isn't necessary, since all cards v1 and v2 are 
            //required to support 512 byte block size, and this is supposed to be
            //the default size selected on cards that support other sizes as well.
            //response = FILEIO_SD_SendMediaCmd_Slow(SET_BLOCKLEN, 0x00000200);    //Set read/write block length to 512 uint8_ts
        }
       
	}


    //Temporarily deselect device
    (*config->csFunc)(1);
    
    //Basic initialization of media is now complete.  The card will now use push/pull
    //outputs with fast drivers.  Therefore, we can now increase SPI speed to 
    //either the maximum of the microcontroller or maximum of media, whichever 
    //is slower.  MMC media is typically good for at least 20Mbps SPI speeds.  
    //SD cards would typically operate at up to 25Mbps or higher SPI speeds.
    spiInitData.mode = 0;
    spiInitData.spibus_mode = SPI_BUS_MODE_2;
    #if defined __XC16__ || defined __XC32__
    	#ifdef __XC32__
            spiInitData.cke = 0;
            if (SYS_CLK_FrequencyPeripheralGet() <= 20000000)
            {
                spiInitData.baudRate = SPICalculateBRG(SYS_CLK_FrequencyPeripheralGet(), 10000);
            }
            else
            {
                spiInitData.baudRate = SPICalculateBRG(SYS_CLK_FrequencyPeripheralGet(), SPI_FREQUENCY);
            }
//    		OpenSPI(SPI_START_CFG_1, SPI_START_CFG_2);
    	#else //else C30 = PIC24/dsPIC devices
            #if defined(DRV_SPI_CONFIG_V2_ENABLED)
                spiInitData.cke = 0;
                spiInitData.primaryPrescale = 0;
                spiInitData.mode = SPI_TRANSFER_MODE_8BIT;
            #else
                spiInitData.cke = 0;
                spiInitData.primaryPrescale = 2;
                spiInitData.secondaryPrescale = 7;
            #endif
        #endif   //#ifdef __XC32__ (and corresponding #else)
    #else //must be PIC18 device
        spiInitData.cke = 0;
        spiInitData.divider = 0;        // 4x divider
    #endif
    spiInitData.channel = config->index;
    DRV_SPI_Initialize(&spiInitData);
    
	(*config->csFunc)(0);

	/* Send the CMD9 to read the CSD register */
    timeout = FILEIO_SD_NCR_TIMEOUT;
    do
    {
        //Send CMD9: Read CSD data structure.
		response = FILEIO_SD_SendCmd(config, FILEIO_SD_SEND_CSD, 0x00);
        timeout--;
    }while((response.r1._byte != 0x00) && (timeout != 0));
    if(timeout != 0x00)
    {
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("CMD9 Successfully processed: Read CSD register.\r\n");
        PrintROMASCIIStringUART("CMD9 response R1 uint8_t = ");
        PrintRAMuint8_tsUART((unsigned char*)&response, 1); 
        UARTSendLineFeedCarriageReturn();
        #endif
    }    
    else
    {
        //Media failed to respond to the read CSD register operation.
        #ifdef __DEBUG_UART  
        PrintROMASCIIStringUART("Timeout occurred while processing CMD9 to read CSD register.\r\n");
        #endif
        
        mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
        (*config->csFunc)(1);
        return &mediaInformation;
    }    

	/* According to the simplified spec, section 7.2.6, the card will respond
	with a standard response token, followed by a data block of 16 uint8_ts
	suffixed with a 16-bit CRC.*/
	index = 0;
	for (count = 0; count < 20u; count ++)
	{
		CSDResponse[index] = DRV_SPI_Get(config->index);
		index++;
		/* Hopefully the first uint8_t is the datatoken, however, some cards do
		not send the response token before the CSD register.*/
		if((count == 0) && (CSDResponse[0] == FILEIO_SD_DATA_START_TOKEN))
		{
			/* As the first uint8_t was the datatoken, we can drop it. */
			index = 0;
		}
	}

    #ifdef __DEBUG_UART  
    PrintROMASCIIStringUART("CSD data structure contains: ");
    PrintRAMuint8_tsUART((unsigned char*)&CSDResponse, 20); 
    UARTSendLineFeedCarriageReturn();
    #endif
    


	//Extract some fields from the response for computing the card capacity.
	//Note: The structure format depends on if it is a CSD V1 or V2 device.
	//Therefore, need to first determine version of the specs that the card 
	//is designed for, before interpreting the individual fields.

	//-------------------------------------------------------------
	//READ_BL_LEN: CSD Structure v1 cards always support 512 uint8_t
	//read and write block lengths.  Some v1 cards may optionally report
	//READ_BL_LEN = 1024 or 2048 bytes (and therefore WRITE_BL_LEN also
	//1024 or 2048).  However, even on these cards, 512 uint8_t partial reads
	//and 512 uint8_t write are required to be supported.
	//On CSD structure v2 cards, it is always required that READ_BL_LEN 
	//(and therefore WRITE_BL_LEN) be 512 uint8_ts, and partial reads and
	//writes are not allowed.
	//Therefore, all cards support 512 uint8_t reads/writes, but only a subset
	//of cards support other sizes.  For best compatibility with all cards,
	//and the simplest firmware design, it is therefore preferable to 
	//simply ignore the READ_BL_LEN and WRITE_BL_LEN values altogether,
	//and simply hardcode the read/write block size as 512 uint8_ts.
	//-------------------------------------------------------------
	gMediaSectorSize = 512u;
	//mediaInformation.sectorSize = gMediaSectorSize;
	mediaInformation.sectorSize = 512u;
	mediaInformation.validityFlags.bits.sectorSize = true;
	//-------------------------------------------------------------

	//Calculate the finalLBA (see SD card physical layer simplified spec 2.0, section 5.3.2).
	//In USB mass storage applications, we will need this information to 
	//correctly respond to SCSI get capacity requests.  Note: method of computing 
	//finalLBA depends on CSD structure spec version (either v1 or v2).
	if(CSDResponse[0] & 0xC0)	//Check CSD_STRUCTURE field for v2+ struct device
	{
		//Must be a v2 device (or a reserved higher version, that doesn't currently exist)

		//Extract the C_SIZE field from the response.  It is a 22-bit number in bit position 69:48.  This is different from v1.  
		//It spans uint8_ts 7, 8, and 9 of the response.
		c_size = (((uint32_t)CSDResponse[7] & 0x3F) << 16) | ((uint16_t)CSDResponse[8] << 8) | CSDResponse[9];
		
		finalLBA = ((uint32_t)(c_size + 1) * (uint16_t)(1024u)) - 1; //-1 on end is correction factor, since LBA = 0 is valid.
	}
	else //if(CSDResponse[0] & 0xC0)	//Check CSD_STRUCTURE field for v1 struct device
	{
		//Must be a v1 device.
		//Extract the C_SIZE field from the response.  It is a 12-bit number in bit position 73:62.  
		//Although it is only a 12-bit number, it spans uint8_ts 6, 7, and 8, since it isn't uint8_t aligned.
		c_size = ((uint32_t)CSDResponse[6] << 16) | ((uint16_t)CSDResponse[7] << 8) | CSDResponse[8];	//Get the uint8_ts in the correct positions
		c_size &= 0x0003FFC0;	//Clear all bits that aren't part of the C_SIZE
		c_size = c_size >> 6;	//Shift value down, so the 12-bit C_SIZE is properly right justified in the uint32_t.
		
		//Extract the C_SIZE_MULT field from the response.  It is a 3-bit number in bit position 49:47.
		c_size_mult = ((uint16_t)((CSDResponse[9] & 0x03) << 1)) | ((uint16_t)((CSDResponse[10] & 0x80) >> 7));

        //Extract the BLOCK_LEN field from the response. It is a 4-bit number in bit position 83:80.
        block_len = CSDResponse[5] & 0x0F;

        block_len = 1 << (block_len - 9); //-9 because we report the size in sectors of 512 uint8_ts each
		
		//Calculate the finalLBA (see SD card physical layer simplified spec 2.0, section 5.3.2).
		//In USB mass storage applications, we will need this information to 
		//correctly respond to SCSI get capacity requests (which will cause FILEIO_SD_CapacityRead() to get called).
		finalLBA = ((uint32_t)(c_size + 1) * (uint16_t)((uint16_t)1 << (c_size_mult + 2)) * block_len) - 1;	//-1 on end is correction factor, since LBA = 0 is valid.		
	}	

    //Turn off CRC7 if we can, might be an invalid cmd on some cards (CMD59)
    //Note: POR default for the media is normally with CRC checking off in SPI 
    //mode anyway, so this is typically redundant.
    FILEIO_SD_SendCmd(config, FILEIO_SD_CRC_ON_OFF, 0x0);

    //Now set the block length to media sector size. It should be already set to this.
    FILEIO_SD_SendCmd(config, FILEIO_SD_SET_BLOCK_LENGTH ,gMediaSectorSize);

    //Deselect media while not actively accessing the card.
    (*config->csFunc)(1);

    #ifdef __DEBUG_UART  
    PrintROMASCIIStringUART("Returning from MediaInitialize() function.\r\n");
    #endif

    //Finished with the SD card initialization sequence.
    if(mediaInformation.errorCode == MEDIA_NO_ERROR)
    {
		__debug("Media Initialized correctly");
        gSDMediaState = FILEIO_SD_STATE_READY_FOR_COMMAND;
    }
    return &mediaInformation;
}//end MediaInitialize

