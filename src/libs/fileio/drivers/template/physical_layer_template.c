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

#include "FSIO.h"
#include "FSDefs.h"
#include "string.h"
#include "TEMPLATEFILE.h"

/*************************************************************************/
/*  Note:  This file is included as a template of a C file for           */
/*         a new physical layer.                                         */
/*************************************************************************/


/******************************************************************************
 * Global Variables
 *****************************************************************************/

static MEDIA_INFORMATION mediaInformation;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

extern void Delayms(BYTE milliseconds);
BYTE MDD_TEMPLATE_MediaInitialize(void);


/******************************************************************************
 * Function:        BYTE MDD_TEMPLATE_MediaDetect(void)
 *
 * PreCondition:    InitIO() function has been executed.
 *
 * Input:           void
 *
 * Output:          TRUE   - Card detected
 *                  FALSE   - No card detected
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
BYTE MDD_TEMPLATE_MediaDetect()
{

/****************************************************************/
/*                    YOUR CODE HERE                            */
/*      If there's a way to detect your device is attached,     */
/*      do it here                                              */
/*                                                              */
/* You return: TRUE if device is detected, FALSE otherwise      */
/****************************************************************/

   // Sample code
   
   if (DEVICE_DETECT_PIN)
      return(TRUE);
   else
      return FALSE;


/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/


}//end MediaDetect


/******************************************************************************
 * Function:        void MDD_TEMPLATE_InitIO(void)
 *
 * PreCondition:    None
 *
 * Input:           void
 *
 * Output:          void
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/

void MDD_TEMPLATE_InitIO (void)
{
/****************************************************************/
/*                    YOUR CODE HERE                            */
/*      Initialize your TRIS bits                               */
/****************************************************************/

   // Example initialization
   WRITESTROBE_TRIS_BITS = OUTPUT;
   READSTROBE_TRIS_BITS = INPUT;
   address_bus_TRIS_BITS = 0x00;
   WRITEPROTECTPIN_TRIS = INPUT;
   DEVICE_DETECT_TRIS = INPUT;

/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/

}
      

/******************************************************************************
 * Function:        MEDIA_INFORMATION * MDD_TEMPLATE_MediaInitialize(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns TRUE if media is initialized, FALSE otherwise
 *
 * Overview:        MediaInitialize initializes the media card and supporting variables.
 *
 * Note:            None
 *****************************************************************************/
MEDIA_INFORMATION * MDD_TEMPLATE_MediaInitialize(void)
{
/****************************************************************/
/*                    YOUR CODE HERE                            */
/*                                                              */
/* You return: TRUE if the device was initialized               */
/*             FALSE otherwise                                  */
/****************************************************************/
   // Example initialization
   BYTE result;

   data_bus_TRIS_BITS = 0x00;

   address_bus = 0;
   data_bus = INITIALIZATION_VALUE;
   WRITESTROBE = 0;
   WRITESTROBE = 1;

   READSTROBE = 0;
   result = data_bus;
   READSTROBE = 1;

   // You can define error codes depending on how your device works
   // 0x00 (MEDIA_NO_ERRORS) indicates success
   if (result == FAILURE)
        mediaInformation.errorCode = 0x00;
   else
        mediaInformation.errorCode = 0x01;

   mediaInformation.validityFlags.value = 0;

/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/

   return &mediaInformation;

}//end MediaInitialize


/******************************************************************************
 * Function:        BYTE MDD_TEMPLATE_SectorRead(DWORD sector_addr, BYTE *buffer)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be stored, see
 *                                'ram_acs.h' for 'block' definition.
 *                                'Block' is dependent on whether internal or
 *                                external memory is used
 *
 * Output:          Returns TRUE if read successful, false otherwise
 *
 * Side Effects:    None
 *
 * Overview:        SectorRead reads 512 bytes of data from the card starting
 *                  at the sector address specified by sector_addr and stores
 *                  them in the location pointed to by 'buffer'.
 *
 * Note:            The device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
BYTE MDD_TEMPLATE_SectorRead(DWORD sector_addr, BYTE* buffer)
{
/****************************************************************/
/*                    YOUR CODE HERE                            */
/* Parameters passed to you:                                    */
/*      sector_addr:  The sector of the card to read from       */
/*      buffer:       The buffer the info will be read to       */
/*                                                              */
/* You return: TRUE if the data is successfully read            */
/*             FALSE otherwise                                  */
/****************************************************************/

   // Example code
   WORD index;
   BYTE status = TRUE;
    BYTE dummyVariable;

   data_bus_TRIS_BITS = 0xFF;

   // Set the address to read from
   // sector_addr is given in # of sectors
   // Multiplying by 512 (the sector size in this example) will
   //     convert it to bytes
   address_bus = sector_addr << 9;   

   for(index = 0; index < MEDIA_SECTOR_SIZE; index++)      //Reads in 512-byte of data
   {      
      // Tell our imaginary device we want to read from it
      READSTROBE = 0;
      // Read the byte, unless the buffer pointer is NULL
        // If it in NULL, just do a dummy read
        // NULL is passed in to provide compatibility with Microchip's USB mass storage code
        if (buffer != NULL)
          buffer[index] = data_bus;
        else
            dummyVariable = data_bus;
      // Reading is done
      READSTROBE = 1;
      // Read the next address
      address_bus++;
   }


   return(status);

/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/

}//end SectorRead

/******************************************************************************
 * Function:        BYTE MDD_TEMPLATE_SectorWrite(DWORD sector_addr, BYTE *buffer, BYTE allowWriteToZero)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be read
 *                  allowWriteToZero - If true, writes to the MBR will be valid
 *
 * Output:          Returns TRUE if write successful, FALSE otherwise
 *
 * Side Effects:    None
 *
 * Overview:        SectorWrite sends 512 bytes of data from the location
 *                  pointed to by 'buffer' to the card starting
 *                  at the sector address specified by sector_addr.
 *
 * Note:            The sample device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
BYTE MDD_TEMPLATE_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero)
{
/****************************************************************/
/*                    YOUR CODE HERE                            */
/* Parameters passed to you:                                    */
/*      sector_addr:  The sector of the card to write to        */
/*      buffer:       The buffer containing info to write       */
/*      allowWriteToZero: Allows overwriting of the MBR         */
/*                                                              */
/* You return: TRUE if the data is successfully written         */
/*             FALSE otherwise                                  */
/****************************************************************/

   // Example code
   WORD index;
   BYTE status = TRUE;

   if (sector_addr == 0 && allowWriteToZero == FALSE)
      status = FALSE;
   else
   {
      data_bus_TRIS_BITS = 0x00;

      // Set the address to write to on your device
      address_bus = sector_addr << 9;

      for(index = 0; index < MEDIA_SECTOR_SIZE; index++)                    //Send 512 bytes of data
      {
         // Write the data to your device
         data_bus = buffer[index];
         WRITESTROBE = 0;
         WRITESTROBE = 1;

         address_bus++;
      }
   } // Not writing to 0 sector

   return(status);


/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/

} //end SectorWrite


/******************************************************************************
 * Function:        BYTE MDD_TEMPLATE_WriteProtectState(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          BYTE    - Returns the status of the "write enabled" pin
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the card is write-protected
 *
 * Note:            None
 *****************************************************************************/

BYTE MDD_TEMPLATE_WriteProtectState(void)
{
/****************************************************************/
/*                    YOUR CODE HERE                            */
/* You return: TRUE if the device is write protected            */
/*             FALSE otherwise                                  */
/****************************************************************/


   return(WRITEPROTECTPIN);


/****************************************************************/
/*                    END OF YOUR CODE                          */
/****************************************************************/

}


/******************************************************************************
 * Function:        BYTE Delayms(void)
 *
 * PreCondition:    None
 *
 * Input:           BYTE millisecons   - Number of ms to delay
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Delays the code- not necessairly accurate
 *
 * Note:            None.
 *****************************************************************************/


void Delayms(BYTE milliseconds)
{
   BYTE    ms;
   DWORD   count;
   
   ms = milliseconds;
   while (ms--)
   {
      count = MILLISECDELAY;

      while (count--)
      {
         ;
      }
   }
   Nop();
   return;
}

