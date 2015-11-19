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

#include "string.h"
#include <fileio_config.h>
#include <fileio.h>
#include <internal_flash.h>

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Global Variables
 *****************************************************************************/

static FILEIO_MEDIA_INFORMATION mediaInformation;

/******************************************************************************
 * Prototypes
 *****************************************************************************/
void EraseBlock(const uint8_t* dest);
void WriteRow(void);
void WriteByte(unsigned char);
FILEIO_MEDIA_INFORMATION * MediaInitialize(void);
void UnlockAndActivate(uint8_t);

//Arbitrary, but "uncommon" value.  Used by UnlockAndActivateWR() to enhance robustness.
#define NVM_UNLOCK_KEY  (uint8_t)0xB5

/******************************************************************************
 * Function:        uint8_t MediaDetect(void* config)
 *
 * PreCondition:    InitIO() function has been executed.
 *
 * Input:           void
 *
 * Output:          true   - Card detected
 *                  false   - No card detected
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
uint8_t FILEIO_InternalFlash_MediaDetect(void* config)
{
    return true;
}//end MediaDetect

/******************************************************************************
 * Function:        uint16_t SectorSizeRead(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          uint16_t - size of the sectors for this physical media.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
uint16_t FILEIO_InternalFlash_SectorSizeRead(void* config)
{
    return FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
}

/******************************************************************************
 * Function:        uint32_t ReadCapacity(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          uint32_t - size of the "disk" - 1 (in terms of sector count).
 *                  Ex: In other words, this function returns the last valid 
 *                  LBA address that may be read/written to.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
uint32_t FILEIO_InternalFlash_CapacityRead(void* config)
{
    //The SCSI READ_CAPACITY command wants to know the last valid LBA address 
    //that the host is allowed to read or write to.  Since LBA addresses start
    //at and include 0, a return value of 0 from this function would mean the 
    //host is allowed to read and write the LBA == 0x00000000, which would be 
    //1 sector worth of capacity.
    //Therefore, the last valid LBA that the host may access is 
    //DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE - 1.
        
    return ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE - 1);
}

/******************************************************************************
 * Function:        uint8_t InitIO(void)
 *
 * PreCondition:    None
 *
 * Input:           void
 *
 * Output:          true   - Card initialized
 *                  false   - Card not initialized
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
uint8_t FILEIO_InternalFlash_InitIO (void* config)
{
    return  true;
}

/******************************************************************************
 * Function:        uint8_t MediaInitialize(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns a pointer to a MEDIA_INFORMATION structure
 *
 * Overview:        MediaInitialize initializes the media card and supporting variables.
 *
 * Note:            None
 *****************************************************************************/
FILEIO_MEDIA_INFORMATION * FILEIO_InternalFlash_MediaInitialize(void* config)
{
    mediaInformation.validityFlags.bits.sectorSize = true;
    mediaInformation.sectorSize = FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
    
    mediaInformation.errorCode = MEDIA_NO_ERROR;
    return &mediaInformation;
}//end MediaInitialize


/******************************************************************************
 * Function:        uint8_t SectorRead(uint32_t sector_addr, uint8_t *buffer)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be stored, see
 *                                'ram_acs.h' for 'block' definition.
 *                                'Block' is dependent on whether internal or
 *                                external memory is used
 *
 * Output:          Returns true if read successful, false otherwise
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
//The flash memory is organized differently on the different microcontroller
//families.  Therefore, multiple versions of this function are implemented.
#if defined(__C30__)    //PIC24 or dsPIC33 device (uint16_t organized flash memory)
uint8_t FILEIO_InternalFlash_SectorRead(void* config, uint32_t sector_addr, uint8_t* buffer)
{
    uint16_t i;
    uint32_t flashAddress;
    uint8_t TBLPAGSave;
    uint16_t temp;
    
    //Error check.  Make sure the host is trying to read from a legitimate
    //address, which corresponds to the MSD volume (and not some other program
    //memory region beyond the end of the MSD volume).
    if(sector_addr >= DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE)
    {
        return false;
    }    
    
    //Save TBLPAG register
    TBLPAGSave = TBLPAG;
   
    //Compute the 24 bit starting address.  Note: this is a word address, but we
    //only store data in and read from the lower word (even LSB).
    //Starting address will always be even, since MasterBootRecord[] uses aligned attribute in declaration.
    flashAddress = (uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_FILES_ADDRESS + (uint32_t)(sector_addr*(uint16_t)FILEIO_CONFIG_MEDIA_SECTOR_SIZE);
    
    //Read a sector worth of data from the flash, and copy to the user specified "buffer".
    for(i = 0; i < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / 2u); i++)
    {
        TBLPAG = (uint8_t)(flashAddress >> 16);   //Load TBLPAG pointer (upper 8 bits of total address.  A sector could get split at
                                        //a 16-bit address boundary, and therefore could exist on two TBLPAG pages.
                                        //Therefore, need to reload TBLPAG every iteration of the for() loop
        temp = __builtin_tblrdl((uint16_t)flashAddress);
        memcpy(buffer, &temp, 2);
        buffer+=2;
        flashAddress += 2u;             //Increment address by 2.  No MSD data stored in the upper uint16_t (which only has one implemented byte anyway).
        
    }   
    
    //Restore TBLPAG register to original value
    TBLPAG = TBLPAGSave;
    
    return true;
}    
#else   //else must be PIC18 or PIC32 device (uint8_t organized flash memory)
uint8_t FILEIO_InternalFlash_SectorRead(void* config, uint32_t sector_addr, uint8_t* buffer)
{
    //Error check.  Make sure the host is trying to read from a legitimate
    //address, which corresponds to the MSD volume (and not some other program
    //memory region beyond the end of the MSD volume).
    if(sector_addr >= DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE)
    {
        return false;
    }   
    
    //Read a sector worth of data, and copy it to the specified RAM "buffer".
    memcpy
    (
        (void*)buffer,
        (const void*)(MASTER_BOOT_RECORD_ADDRESS + (sector_addr * FILEIO_CONFIG_MEDIA_SECTOR_SIZE)),
        FILEIO_CONFIG_MEDIA_SECTOR_SIZE
    );

	return true;
}//end SectorRead
#endif


/******************************************************************************
 * Function:        uint8_t SectorWrite(uint32_t sector_addr, uint8_t *buffer, uint8_t allowWriteToZero)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be read
 *                  allowWriteToZero - If true, writes to the MBR will be valid
 *
 * Output:          Returns true if write successful, false otherwise
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
#if defined(__XC8) || defined(__18CXX)
    #if defined(__18CXX)
        #pragma udata myFileBuffer
    #endif
    volatile unsigned char file_buffer[DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE];
#elif defined (__dsPIC33E__) || defined (__PIC24E__)
    volatile unsigned int file_buffer[DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE] __attribute__((far));
#else
    volatile unsigned char file_buffer[DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE] __attribute__((far,aligned));
#endif

#if defined(__18CXX)
    #pragma udata
#endif

#define INTERNAL_FLASH_PROGRAM_WORD        0x4003
#define INTERNAL_FLASH_ERASE               0x4042
#define INTERNAL_FLASH_PROGRAM_PAGE        0x4001


#if defined(__C32__)
    #define PTR_SIZE uint32_t
#elif defined(__XC8)
    #define PTR_SIZE uint32_t
#elif defined(__18CXX)
    #define PTR_SIZE UINT24
#else
    #define PTR_SIZE uint16_t
#endif
const uint8_t *FileAddress = 0;


#if defined(__C30__)
uint8_t FILEIO_InternalFlash_SectorWrite(void* config, uint32_t sector_addr, uint8_t* buffer, uint8_t allowWriteToZero)
{
#if !defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_WRITE_PROTECT)
    uint16_t i;
    uint8_t j;
    uint16_t offset;
    uint32_t flashAddress;
    uint16_t TBLPAGSave;


    //First, error check the resulting address, to make sure the MSD host isn't trying 
    //to erase/program illegal LBAs that are not part of the designated MSD volume space.
    if(sector_addr >= DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE)
    {
        return false;
    }  

    TBLPAGSave = TBLPAG;
    
#if defined (__dsPIC33E__) || defined (__PIC24E__)
    

    // First, save the contents of the entire erase page.  
    // To do this, we need to get a pointer to the start of the erase page.
    // AND mask 0xFFFFF800 is to clear the lower bits, 
    // so we go back to the start of the erase page.
    
    flashAddress = ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_FILES_ADDRESS + (uint32_t)(sector_addr*FILEIO_CONFIG_MEDIA_SECTOR_SIZE))
                & (uint32_t)0xFFFFF800;
    
    //Now save all of the contents of the erase page.
    TBLPAG = (uint8_t)(flashAddress >> 16);
    for(i = 0; i < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE;i++)
    {
        file_buffer[i] = __builtin_tblrdl((uint16_t)flashAddress + (2 * i));
    }    

    // Now we want to overwrite the file_buffer[] contents 
    // for the sector that we are trying to write to.
    // The lower 2 bits of the helps to determine this.
   
    offset = 0x200 * (uint8_t)(sector_addr & 0x3);

    //Overwrite the file_buffer[] RAM contents for the sector that we are trying to write to.
    for(i = 0; i < FILEIO_CONFIG_MEDIA_SECTOR_SIZE; i++)
    {
        *((unsigned char *)file_buffer + offset + i) = *buffer++;
    }
#else

     //First, save the contents of the entire erase page.  To do this, we need to get a pointer to the start of the erase page.
    flashAddress = ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_FILES_ADDRESS + (uint32_t)(sector_addr*FILEIO_CONFIG_MEDIA_SECTOR_SIZE)) & (uint32_t)0xFFFFFC00;  //AND mask 0xFFFFFC00 is to clear the lower bits, so we go back to the start of the erase page.
    //Now save all of the contents of the erase page.
    for(i = 0; i < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE;)
    {
        TBLPAG = (uint8_t)(flashAddress >> 16);
        *(uint16_t*)&file_buffer[i] = __builtin_tblrdl((uint16_t)flashAddress);
        flashAddress += 2u;    //Skipping upper word.  Don't care about the implemented byte/don't use it when programming or reading from the sector.
        i += 2u;
    }    

    //Now we want to overwrite the file_buffer[] contents for the sector that we are trying to write to.
    //Need to figure out if the buffer[] data goes in the upper sector or the lower sector of the file_buffer[]
    if(sector_addr & 0x00000001)
    {
        //Odd sector address, must be the high file_buffer[] sector
        offset = FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
    }
    else
    {
        offset = 0;
    }        

    //Overwrite the file_buffer[] RAM contents for the sector that we are trying to write to.
    for(i = 0; i < FILEIO_CONFIG_MEDIA_SECTOR_SIZE; i++)
    {
        file_buffer[offset + i] = *buffer++;
    }
    #endif
    

#if defined(__dsPIC33E__) || defined (__PIC24E__)

    int gieBkUp;

    //Now erase the entire erase page of flash memory.  
    //First we need to calculate the actual flash memory 
    //address of the erase page. 
    
    gieBkUp = INTCON2bits.GIE;
    INTCON2bits.GIE = 0; // Disable interrupts
    NVMADRU = (uint16_t)(flashAddress >> 16);
    NVMADR = (uint16_t)(flashAddress & 0xFFFF);
    NVMCON = 0x4003;    // This value will erase a page.
    __builtin_write_NVM();
    INTCON2bits.GIE = gieBkUp; // Enable interrupts

    //Now reprogram the erase page with previously obtained contents of the file_buffer[]
    //We only write to the even flash word addresses, the odd word addresses are left blank.  
    //Therefore, we only store 2 bytes of application data for every 2 flash memory word addresses.
    //This "wastes" 1/3 of the flash memory, but it provides extra protection from accidentally executing
    //the data.  It also allows quick/convenient PSV access when reading back the flash contents.

    TBLPAG = 0xFA;
    j = 0;
    for(i = 0; i < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE;i++)
    {

       //
        __builtin_tblwtl((j * 2), file_buffer[i]);
        __builtin_tblwth((j * 2), 0);
           
        j ++;

        //Check if we have reached a program block size boundary.  If so, program the last 128 
        //useful bytes (192 bytes total, but 64 of those are filled with '0' filler bytes).
        if(j >= 128u)
        {
            j = j - 128u;
            NVMADRU = (uint16_t)(flashAddress >> 16);
            NVMADR = (uint16_t)(flashAddress & 0xFFFF);
            NVMCON = 0x4002;
            gieBkUp = INTCON2bits.GIE;
            INTCON2bits.GIE = 0; // Disable interrupts
            __builtin_write_NVM();
            INTCON2bits.GIE = gieBkUp; // Enable interrupts
            flashAddress += 256;
        }    
    } 
#else 
    //Now erase the entire erase page of flash memory.  
    //First we need to calculate the actual flash memory address of the erase page.  The starting address of the erase page is as follows:
    flashAddress = ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_FILES_ADDRESS + (uint32_t)(sector_addr*FILEIO_CONFIG_MEDIA_SECTOR_SIZE)) & (uint32_t)0xFFFFFC00;

    //Peform NVM erase operation.
    NVMCON = INTERNAL_FLASH_ERASE;				    //Page erase on next WR
    __builtin_tblwtl((uint16_t)flashAddress, 0xFFFF);   //Perform dummy write to load address of erase page
    UnlockAndActivate(NVM_UNLOCK_KEY);

    //Now reprogram the erase page with previously obtained contents of the file_buffer[]
    //We only write to the even flash word addresses, the odd word addresses are left blank.  
    //Therefore, we only store 2 bytes of application data for every 2 flash memory word addresses.
    //This "wastes" 1/3 of the flash memory, but it provides extra protection from accidentally executing
    //the data.  It also allows quick/convenient PSV access when reading back the flash contents.
    NVMCON = INTERNAL_FLASH_PROGRAM_PAGE;
    j = 0;
    for(i = 0; i < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE;)
    {
        TBLPAG = (uint8_t)(flashAddress >> 16);
        __builtin_tblwtl((uint16_t)flashAddress, *((uint16_t*)&file_buffer[i]));
        flashAddress++;       
        __builtin_tblwth((uint16_t)flashAddress, 0);
        flashAddress++;       

        i += 2;
        j += 2;

        //Check if we have reached a program block size boundary.  If so, program the last 128 
        //useful bytes (192 bytes total, but 64 of those are filled with '0' filler bytes).
        if(j >= 128u)
        {
            j = j - 128u;
            asm("DISI #16");					//Disable interrupts for next few instructions for unlock sequence
            __builtin_write_NVM();                        
        }    
    }    
#endif

    TBLPAG = TBLPAGSave;   
    return true;
#else
    return true;
#endif

}    
#else   //else must be PIC18 or PIC32 device
uint8_t FILEIO_InternalFlash_SectorWrite(void* config, uint32_t sector_addr, uint8_t* buffer, uint8_t allowWriteToZero)
{
    #if !defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_WRITE_PROTECT)
        const uint8_t* dest;
        bool foundDifference;
        uint16_t blockCounter;
        uint16_t sectorCounter;

        #if defined(__XC8) || defined(__18CXX)
            uint8_t* p;
        #endif

        //First, error check the resulting address, to make sure the MSD host isn't trying 
        //to erase/program illegal LBAs that are not part of the designated MSD volume space.
        if(sector_addr >= DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE)
        {
            return false;
        }  

        //Compute pointer to location in flash memory we should modify
        dest = (const uint8_t*)(MASTER_BOOT_RECORD_ADDRESS + (sector_addr * FILEIO_CONFIG_MEDIA_SECTOR_SIZE));

        sectorCounter = 0;
        //Loop that actually does the flash programming, until all of the
        //intended sector data has been programmed
        while(sectorCounter < FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
        {
            //First, read the contents of flash to see if they already match what the
            //host is trying to write.  If every byte already matches perfectly,
            //we can save flash endurance by not actually performing the reprogramming
            //operation.
            foundDifference = false;
            for(blockCounter = 0; blockCounter < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE; blockCounter++)
            {
                if(dest[sectorCounter] != buffer[sectorCounter])
                {
                    foundDifference = true;
                    sectorCounter -= blockCounter;
                    break;
                }
                sectorCounter++;
            }

            //If the existing flash memory contents are different from what is waiting
            //in the RAM buffer to be programmed.  We will need to do some flash reprogramming.
            if(foundDifference == true)
            {
                uint8_t i;
                PTR_SIZE address;

                //Check to see which is bigger, the flash memory minimum erase page size, or the sector size.
                #if (DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE >= FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
                    //The hardware erases more flash memory than the amount of a sector that
                    //we are programming.  Therefore, we will have to use a three step process:
                    //1. Read out the flash memory contents that are part of the erase page (but we don't need to modify)
                    //   and save it temporarily to a RAM buffer.
                    //2. Erase the flash memory page (which blows away multiple sectors worth of data in flash)
                    //3. Reprogram both the intended sector data, and the unmodified flash data that we didn't want to
                    //   modify, but had to temporarily erase (since it was sharing the erase page with our intended write location).

                    //Compute a pointer to the first byte on the erase page of interest
                    address = ((PTR_SIZE)(dest + sectorCounter) & ~((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));

                    //Read out the entire contents of the flash memory erase page of interest and save to RAM.
                    memcpy
                    (
                        (void*)file_buffer,
                        (const void*)address,
                        DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE
                    );

                    //Now erase the flash memory page
                    EraseBlock((const uint8_t*)address);

                    //Compute a pointer into the RAM buffer with the erased flash contents,
                    //to where we want to replace the existing data with the new data from the host.
                    address = ((PTR_SIZE)(dest + sectorCounter) & ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));

                    //Overwrite part of the erased page RAM buffer with the new data being
                    //written from the host
                    memcpy
                    (
                        (void*)(&file_buffer[address]),
                        (void*)buffer,
                        FILEIO_CONFIG_MEDIA_SECTOR_SIZE
                    );

                #else
                    //The erase page size is small enough, we don't have to (temporarily) erase
                    //any data, other than the specific flash region that we want to re-program with new data.

                    //Move sector counter to the first byte on the erase page of interest
                    sectorCounter &= ~(DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1);

                    //Compute a pointer to the first byte of flash program memory on the erase page of interest
                    address = (PTR_SIZE)dest + sectorCounter;

                    //Erase a page of flash memory
                    EraseBlock((const uint8_t*)address);

                    memcpy
                    (
                        (void*)&file_buffer[0],
                        (void*)(buffer+sectorCounter),
                        DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE
                    );
                #endif


                //Compute the number of write blocks that are in the erase page.
                i = DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE / DRV_FILEIO_INTERNAL_FLASH_CONFIG_WRITE_BLOCK_SIZE;

                #if defined(__XC8) || defined(__18CXX)
                    p = (uint8_t*)&file_buffer[0];
                    TBLPTR = ((PTR_SIZE)(dest + sectorCounter) & ~((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));
                #endif

                //Commit each write block worth of data to the flash memory, one block at a time
                while(i-- > 0)
                {
                    //Write a block of the RAM bufferred data to the programming latches
                    for(blockCounter = 0; blockCounter < DRV_FILEIO_INTERNAL_FLASH_CONFIG_WRITE_BLOCK_SIZE; blockCounter++)
                    {
                        //Write the data
                        #if defined(__XC8)
                            TABLAT = *p++;
                            #asm
                                tblwtpostinc
                            #endasm
                            sectorCounter++;
                        #elif defined(__18CXX)
                            TABLAT = *p++;
                            _asm tblwtpostinc _endasm
                            sectorCounter++;
                        #endif

                        #if defined(__C32__)
                            NVMWriteWord((uint32_t*)KVA_TO_PA(FileAddress), *((uint32_t*)&file_buffer[sectorCounter]));
                            FileAddress += 4;
                            sectorCounter += 4;
                        #endif
                    }

                    //Now commit/write the block of data from the programming latches into the flash memory
                    #if defined(__XC8)
                        // Start the write process: for PIC18, first need to reposition tblptr back into memory block that we want to write to.
                        #asm 
                            tblrdpostdec 
                        #endasm

                        // Write flash memory, enable write control.
                        EECON1 = 0x84;
                        UnlockAndActivate(NVM_UNLOCK_KEY);
                        TBLPTR++;                    
                    #elif defined(__18CXX)
                        // Start the write process: for PIC18, first need to reposition tblptr back into memory block that we want to write to.
                         _asm tblrdpostdec _endasm

                        // Write flash memory, enable write control.
                        EECON1 = 0x84;
                        UnlockAndActivate(NVM_UNLOCK_KEY);
                        TBLPTR++;
                    #endif
                }//while(i-- > 0)
            }//if(foundDifference == true)
        }//while(sectorCounter < FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
    	return true;
    #else
        return true;
    #endif
} //end SectorWrite
#endif  //#if defined(__C30__)



#if !defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_WRITE_PROTECT)
void EraseBlock(const uint8_t* dest)
{
    #if defined(__XC8) || defined(__18CXX)
        TBLPTR = (unsigned long)dest;

        //Erase the current block
        EECON1 = 0x94;
        UnlockAndActivate(NVM_UNLOCK_KEY);
    #endif

    #if defined(__C32__)
        FileAddress = dest;
        NVMErasePage((uint8_t *)KVA_TO_PA(dest));
    #endif
}


//------------------------------------------------------------------------------
#if defined(__XC16__)
    #pragma message "Double click this message and read inline code comments.  For production designs, recommend adding application specific robustness features here."
#else
    #warning "Double click this message and read inline code comments.  For production designs, recommend adding application specific robustness features here."
#endif
//Function: void UnlockAndActivate(uint8_t UnlockKey)
//Description: Activates and initiates a flash memory self erase or program 
//operation.  Useful for writing to the MSD drive volume.
//Note: Self erase/writes to flash memory could potentially corrupt the
//firmware of the application, if the unlock sequence is ever executed
//unintentionally, or if the table pointer is pointing to an invalid
//range (not inside the MSD volume range).  Therefore, in order to ensure
//a fully reliable design that is suitable for mass production, it is strongly
//recommended to implement several robustness checks prior to actually
//performing any self erase/program unlock sequence.  See additional inline 
//code comments.
//------------------------------------------------------------------------------
void UnlockAndActivate(uint8_t UnlockKey)
{
    #if defined(__XC8) || defined(__18CXX)
        uint8_t InterruptEnableSave;
    #endif
      
    //Should verify that the voltage on Vdd/Vddcore is high enough to meet
    //the datasheet minimum voltage vs. frequency graph for the device.
    //If the microcontroller is "overclocked" (ex: by running at maximum rated
    //frequency, but then not suppling enough voltage to meet the datasheet
    //voltage vs. frequency graph), errant code execution could occur.  It is
    //therefore strongly recommended to check the voltage prior to performing a 
    //flash self erase/write unlock sequence.  If the voltage is too low to meet
    //the voltage vs. frequency graph in the datasheet, the firmware should not 
    //inititate a self erase/program operation, and instead it should either:
    //1.  Clock switch to a lower frequency that does meet the voltage/frequency graph.  Or,
    //2.  Put the microcontroller to Sleep mode.
    
    //The method used to measure Vdd and/or Vddcore will depend upon the 
    //microcontroller model and the module features available in the device, but
    //several options are available on many of the microcontrollers, ex:
    //1.  HLVD module
    //2.  WDTCON<LVDSTAT> indicator bit
    //3.  Perform ADC operation, with the VBG channel selected, using Vdd/Vss as 
    //      references to the ADC.  Then perform math operations to calculate the Vdd.
    //      On some micros, the ADC can also measure the Vddcore voltage, allowing
    //      the firmware to calculate the absolute Vddcore voltage, if it has already
    //      calculated and knows the ADC reference voltage.
    //4.  Use integrated general purpose comparator(s) to sense Vdd/Vddcore voltage
    //      is above proper threshold.
    //5.  If the micrcontroller implements a user adjustable BOR circuit, enable
    //      it and set the trip point high enough to avoid overclocking altogether.
    
    //Example psuedo code.  Exact implementation will be application specific.
    //Please implement appropriate code that best meets your application requirements.
    //if(GetVddcoreVoltage() < MIN_ALLOWED_VOLTAGE)
    //{
    //    ClockSwitchToSafeFrequencyForGivenVoltage();    //Or even better, go to sleep mode.
    //    return;       
    //}    


    //Should also verify the TBLPTR is pointing to a valid range (part of the MSD
    //volume, and not a part of the application firmware space).
    //Example code for PIC18 (commented out since the actual address range is 
    //application specific):
    //if((TBLPTR > MSD_VOLUME_MAX_ADDRESS) || (TBLPTR < MSD_VOLUME_START_ADDRESS)) 
    //{
    //    return;
    //}  
    
    //Verify the UnlockKey is the correct value, to make sure this function is 
    //getting executed intentionally, from a calling function that knew it
    //should pass the correct NVM_UNLOCK_KEY value to this function.
    //If this function got executed unintentionally, then it would be unlikely
    //that the UnlockKey variable would have been loaded with the proper value.
    if(UnlockKey != NVM_UNLOCK_KEY)
    {
        #if defined(__XC8) || defined(__18CXX)
            EECON1bits.WREN = 0;
        #endif
        return;
    }    
    
  
    //We passed the robustness checks.  Time to Erase/Write the flash memory.
    #if defined(__XC8) || defined(__18CXX)
        InterruptEnableSave = INTCON;
        INTCONbits.GIEH = 0;    //Disable interrupts for unlock sequence.
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;      //CPU stalls until flash erase/write is complete
        EECON1bits.WREN = 0;    //Good practice to disable any further writes now.
        //Safe to re-enable interrupts now, if they were previously enabled.
        if(InterruptEnableSave & 0x80)  //Check if GIEH was previously set
        {
            INTCONbits.GIEH = 1;
        }    
    #endif    
    #if defined(__C30__)
    	asm("DISI #16");					//Disable interrupts for next few instructions for unlock sequence
	    __builtin_write_NVM();
    #endif
        
}    
#endif  //end of #if !defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_WRITE_PROTECT)


/******************************************************************************
 * Function:        uint8_t WriteProtectState(void* config)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          uint8_t    - Returns the status of the "write enabled" pin
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the card is write-protected
 *
 * Note:            None
 *****************************************************************************/

uint8_t FILEIO_InternalFlash_WriteProtectStateGet(void* config)
{
    #if defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_WRITE_PROTECT)
        return true;
    #else
	    return false;
    #endif
}

