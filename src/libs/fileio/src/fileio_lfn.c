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

#include "system_config.h"
#include "system.h"
#include "fileio_lfn.h"
#include "../src/fileio_private_lfn.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t ReadRam32bit(uint8_t * pBuffer, uint16_t index);
uint16_t ReadRam16bit (uint8_t * pBuffer, uint16_t index);

/*****************************************************************************/
/*                         Global Variables                                  */
/*****************************************************************************/

FILEIO_DRIVE gDriveArray[FILEIO_CONFIG_MAX_DRIVES];
uint8_t gDriveSlotOpen[FILEIO_CONFIG_MAX_DRIVES];

FILEIO_TimestampGet timestampGet;

uint16_t lfnBuffer[256];

#if defined (__XC16__) || defined (__XC32__)
    #if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        uint8_t __attribute__ ((aligned(4)))   gDataBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];      // The global data sector buffer
        uint8_t __attribute__ ((aligned(4)))   gFATBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];       // The global FAT sector buffer
        FILEIO_BUFFER_STATUS bufferStatus;                                          // Status of the buffer contents (and buffer ownership)
    #else
        uint8_t __attribute__ ((aligned(4)))   gDataBuffer[FILEIO_CONFIG_MAX_DRIVES][FILEIO_CONFIG_MEDIA_SECTOR_SIZE];     // The global data sector buffer
        uint8_t __attribute__ ((aligned(4)))   gFATBuffer[FILEIO_CONFIG_MAX_DRIVES][FILEIO_CONFIG_MEDIA_SECTOR_SIZE];      // The global FAT sector buffer
        FILEIO_BUFFER_STATUS bufferStatus[FILEIO_CONFIG_MAX_DRIVES];
    #endif
#else
    #if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        uint8_t gDataBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];      // The global data sector buffer
        uint8_t gFATBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];       // The global FAT sector buffer
        FILEIO_BUFFER_STATUS bufferStatus;                                          // Status of the buffer contents (and buffer ownership)
    #else
        uint8_t gDataBuffer[FILEIO_CONFIG_MAX_DRIVES][FILEIO_CONFIG_MEDIA_SECTOR_SIZE];     // The global data sector buffer
        uint8_t gFATBuffer[FILEIO_CONFIG_MAX_DRIVES][FILEIO_CONFIG_MEDIA_SECTOR_SIZE];      // The global FAT sector buffer
        FILEIO_BUFFER_STATUS bufferStatus[FILEIO_CONFIG_MAX_DRIVES];
    #endif
#endif

struct
{
    FILEIO_DIRECTORY currentWorkingDirectory;
} globalParameters;

/************************************************************************************/
/*                               Prototypes                                         */
/************************************************************************************/

void FILEIO_RegisterTimestampGet (FILEIO_TimestampGet timestampFunction)
{
    timestampGet = timestampFunction;
}

int FILEIO_Initialize (void)
{
    int i;

    for (i = 0; i < FILEIO_CONFIG_MAX_DRIVES; i++)
    {
        gDriveSlotOpen[i] = true;
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        gDriveArray[i].dataBuffer = &gDataBuffer[0];
        gDriveArray[i].fatBuffer = &gFATBuffer[0];
        gDriveArray[i].bufferStatusPtr = &bufferStatus;
#else
        gDriveArray[i].dataBuffer = &gDataBuffer[i][0];
        gDriveArray[i].fatBuffer = &gFATBuffer[i][0];
        gDriveArray[i].bufferStatusPtr = &bufferStatus[i];
        bufferStatus[i].flags.dataBufferNeedsWrite = false;
        bufferStatus[i].flags.fatBufferNeedsWrite = false;
        bufferStatus[i].dataBufferCachedSector = 0xFFFFFFFF;
        bufferStatus[i].fatBufferCachedSector = 0xFFFFFFFF;
#endif
    }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    bufferStatus.driveOwner = NULL;
    bufferStatus.flags.dataBufferNeedsWrite = false;
    bufferStatus.flags.fatBufferNeedsWrite = false;
    bufferStatus.dataBufferCachedSector = 0xFFFFFFFF;
    bufferStatus.fatBufferCachedSector = 0xFFFFFFFF;
#endif
    
    globalParameters.currentWorkingDirectory.drive = 0;
    globalParameters.currentWorkingDirectory.cluster = 0;

    return true;
}

int FILEIO_Reinitialize (void)
{
    return FILEIO_Initialize();
}

bool FILEIO_MediaDetect (const FILEIO_DRIVE_CONFIG * driveConfig, void * mediaParameters)
{
    return (*driveConfig->funcMediaDetect)(mediaParameters);
}

FILEIO_DRIVE * FILEIO_CharToDrive (uint16_t driveId)
{
    uint8_t i;

    for (i = 0; i < FILEIO_CONFIG_MAX_DRIVES; i++)
    {
        if ((gDriveSlotOpen[i] == false) && (gDriveArray[i].driveId == driveId))
        {
            return &gDriveArray[i];
        }
    }

    return NULL;
}

FILEIO_FILE_SYSTEM_TYPE FILEIO_FileSystemTypeGet (uint16_t driveId)
{
    FILEIO_DRIVE * drive = FILEIO_CharToDrive (driveId);

    if (drive == NULL)
    {
        return FILEIO_FILE_SYSTEM_TYPE_NONE;
    }

    return drive->type;
}

FILEIO_ERROR_TYPE FILEIO_DriveMount (uint16_t driveId, const FILEIO_DRIVE_CONFIG * driveConfig, void * mediaParameters)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DRIVE * drive;
    uint8_t i;
    FILEIO_MEDIA_INFORMATION * mediaInformation;
    
    drive = FILEIO_CharToDrive (driveId);

    if (drive == NULL)
    {
        for (i = 0; i < FILEIO_CONFIG_MAX_DRIVES; i++)
        {
            if (gDriveSlotOpen[i])
            {
                gDriveSlotOpen[i] = false;
                gDriveArray[i].driveId = driveId;
                gDriveArray[i].driveConfig = driveConfig;
                drive = &gDriveArray[i];
                break;
            }
        }
    }
    
    if (drive == NULL)
    {
        return FILEIO_ERROR_TOO_MANY_DRIVES_OPEN;
    }

    // Reinitialize the drive cache information
    // This will force the library to recache sectors if a drive is unmounted and re-mounted
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (drive->bufferStatusPtr->driveOwner == drive)
    {
        drive->bufferStatusPtr->driveOwnerdriveOwner = NULL;
    }
#else
    bufferStatus[i].flags.dataBufferNeedsWrite = false;
    bufferStatus[i].flags.fatBufferNeedsWrite = false;
    bufferStatus[i].dataBufferCachedSector = 0xFFFFFFFF;
    bufferStatus[i].fatBufferCachedSector = 0xFFFFFFFF;
#endif

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (drive) != FILEIO_RESULT_SUCCESS)
    {
        return FILEIO_ERROR_WRITE;
    }
#endif

    drive->mediaParameters = mediaParameters;

    if(driveConfig->funcIOInit != NULL)
    {
        (*driveConfig->funcIOInit)(mediaParameters);
    }

    mediaInformation = (*driveConfig->funcMediaInit)(mediaParameters);
    if (mediaInformation->errorCode != MEDIA_NO_ERROR)
    {
        error = FILEIO_ERROR_INIT_ERROR;
    }
    else
    {
        if (mediaInformation->validityFlags.bits.sectorSize)
        {
            drive->sectorSize = mediaInformation->sectorSize;
            if (mediaInformation->sectorSize > FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
            {
                error = FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE;
            }
        }
    }

    // Load the Master Boot Record (partition)
    if (error == FILEIO_ERROR_NONE)
    {
        if ((error = FILEIO_LoadMBR (drive)) == FILEIO_ERROR_NONE)
        {
            // Load the boot sector
            error = FILEIO_LoadBootSector(drive);
        }
    }

    if (error == FILEIO_ERROR_NONE)
    {
        // If this is the first drive we're mounting, set its root as the current working directory
        if (globalParameters.currentWorkingDirectory.drive == 0)
        {
            globalParameters.currentWorkingDirectory.drive = drive;
            globalParameters.currentWorkingDirectory.cluster = drive->firstRootCluster;
        }
    }
    else
    {
        gDriveArray[i].driveId = 0;
        gDriveSlotOpen[i] = true;
    }

    return error;
}

FILEIO_ERROR_TYPE FILEIO_LoadMBR (FILEIO_DRIVE * drive)
{
    FILEIO_MASTER_BOOT_RECORD * ptrMbr;
    uint8_t type;
    bool hasMbr = true;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_BOOT_SECTOR * ptrBootSector;

     // Get the partition table from the MBR
    if ( (*drive->driveConfig->funcSectorRead) (drive->mediaParameters, FILEIO_MEDIA_SECTOR_MBR, drive->dataBuffer) != true)
    {
        error = FILEIO_ERROR_BAD_SECTOR_READ;
    }
    else
    {
        drive->bufferStatusPtr->dataBufferCachedSector = FILEIO_MEDIA_SECTOR_MBR;
        // Check if the card has no MBR
        ptrBootSector = (FILEIO_BOOT_SECTOR *)drive->dataBuffer;

        if ((ptrBootSector->signature0 == FILEIO_FAT_GOOD_SIGN_0) && (ptrBootSector->signature1 == FILEIO_FAT_GOOD_SIGN_1))
        {
         // Technically, the OEM name is not for indication
         // The alternative is to read the CIS from attribute
         // memory.  See the PCMCIA metaformat for more details
/*    #if defined (__XC16__) || defined (__XC32__)
            if ((*(drive->dataBuffer + BSI_FSTYPE) == 'F') && \
                (*(drive->dataBuffer + BSI_FSTYPE + 1) == 'A') && \
                (*(drive->dataBuffer + BSI_FSTYPE + 2) == 'T') && \
                (*(drive->dataBuffer + BSI_FSTYPE + 3) == '1') && \
                (*(drive->dataBuffer + BSI_BOOTSIG) == 0x29))
#else*/
            if ((ptrBootSector->biosParameterBlock.fat16.fileSystemType[0] == 'F') && \
                (ptrBootSector->biosParameterBlock.fat16.fileSystemType[1] == 'A') && \
                (ptrBootSector->biosParameterBlock.fat16.fileSystemType[2] == 'T') && \
                (ptrBootSector->biosParameterBlock.fat16.fileSystemType[3] == '1') && \
                (ptrBootSector->biosParameterBlock.fat16.bootSignature == 0x29))
//    #endif
             {
                drive->firstPartitionSector = 0;
                drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT16;
                hasMbr = false;
             }
             else
             {
#if defined (__XC16__) || defined (__XC32__)
                if ((*(drive->dataBuffer + BSI_FAT32_FSTYPE ) == 'F') && \
                    (*(drive->dataBuffer + BSI_FAT32_FSTYPE + 1 ) == 'A') && \
                    (*(drive->dataBuffer + BSI_FAT32_FSTYPE + 2 ) == 'T') && \
                    (*(drive->dataBuffer + BSI_FAT32_FSTYPE + 3 ) == '3') && \
                    (*(drive->dataBuffer + BSI_FAT32_BOOTSIG) == 0x29))
#else
                if ((ptrBootSector->biosParameterBlock.fat32.fileSystemType[0] == 'F') && \
                    (ptrBootSector->biosParameterBlock.fat32.fileSystemType[1] == 'A') && \
                    (ptrBootSector->biosParameterBlock.fat32.fileSystemType[2] == 'T') && \
                    (ptrBootSector->biosParameterBlock.fat32.fileSystemType[3] == '3') && \
                    (ptrBootSector->biosParameterBlock.fat32.bootSignature == 0x29))
#endif
                {
                    drive->firstPartitionSector = 0;
                    drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT32;
                    hasMbr = false;
                }
            }
        }

        // If the first sector isn't a boot sector, parse the actual MBR
        if (hasMbr)
        {
            // assign it the partition table structure
            ptrMbr = (FILEIO_MASTER_BOOT_RECORD *)drive->dataBuffer;

            // Ensure its good
            if((ptrMbr->signature0 != FILEIO_FAT_GOOD_SIGN_0) || (ptrMbr->signature1 != FILEIO_FAT_GOOD_SIGN_1))
            {
                error = FILEIO_ERROR_BAD_PARTITION;
            }
            else
            {
                uint8_t j;
                FILEIO_MBR_PARTITION_TABLE_ENTRY * ptrEntry = &ptrMbr->partition0;

                drive->type = FILEIO_FILE_SYSTEM_TYPE_NONE;
                for(j = 0; j < 4; j++)
                {
                    /*    Valid Master Boot Record Loaded   */

                    // Get the 32 bit offset to the first partition
                    drive->firstPartitionSector = ptrEntry->lbaFirstSector;

                    // check if the partition type is acceptable
                    type = ptrEntry->fileSystemDescriptor;

                    switch (type)
                    {
                        case 0x01:
                            drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT12;
                            break;
                        case 0x04:
                        case 0x06:
                        case 0x0E:
                            drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT16;
                            break;
                        case 0x0B:
                        case 0x0C:
                                drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT32;    // FAT32 is supported too
                                break;
                    } // switch

                    if (drive->type != FILEIO_FILE_SYSTEM_TYPE_NONE)
                    {
                        break;
                    }

                    /* If we are here, we didn't find a matching partition.  We
                       should increment to the next partition table entry */
                    ptrEntry++;
                }

                if (drive->type == FILEIO_FILE_SYSTEM_TYPE_NONE)
                {
                    error = FILEIO_ERROR_UNSUPPORTED_FS;
                }
            }
        }
    }

    return error;
}

FILEIO_ERROR_TYPE FILEIO_LoadBootSector (FILEIO_DRIVE * drive)
{
    FILEIO_BOOT_SECTOR * ptrBootSector;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    uint32_t rootDirectorySectors;
    uint32_t totalSectors;
    uint32_t dataSectors;
    uint16_t bytesPerSector;
    uint16_t reservedSectorCount;
    bool triedSpecifiedBackupBootSec = false;
    bool triedBackupBootSecAtAddress6 = false;

    // Get the Boot sector
    if ( (*drive->driveConfig->funcSectorRead) (drive->mediaParameters, drive->firstPartitionSector, drive->dataBuffer) != true)
    {
        error = FILEIO_ERROR_BAD_SECTOR_READ;
    }
    else
    {
        drive->bufferStatusPtr->dataBufferCachedSector = drive->firstPartitionSector;
        ptrBootSector = (FILEIO_BOOT_SECTOR *)drive->dataBuffer;

        do      //test each possible boot sector (FAT32 can have backup boot sectors)
        {

            //Verify the Boot Sector has a valid signature
            if((ptrBootSector->signature0 != FILEIO_FAT_GOOD_SIGN_0) || (ptrBootSector->signature1 != FILEIO_FAT_GOOD_SIGN_1))
            {
                error = FILEIO_ERROR_NOT_FORMATTED;
            }
            else
            {
                do      //loop just to allow a break to jump out of this section of code
                {
                    #ifdef __XC8__
                    // Load count of sectors per cluster
                    drive->sectorsPerCluster = ptrBootSector->biosParameterBlock.fat16.sectorsPerCluster;
                    // Load the sector number of the first FAT sector
                    drive->firstFatSector = drive->firstPartitionSector + ptrBootSector->biosParameterBlock.fat16.reservedSectorCount;
                    // Load the count of FAT tables
                    drive->fatCopyCount = ptrBootSector->biosParameterBlock.fat16.fatCount;
                    // Load the size of the FATs
                    drive->fatSectorCount = ptrBootSector->biosParameterBlock.fat16.sectorsPerFat;
                    if(drive->fatSectorCount == 0)
                    {
                        drive->fatSectorCount  = ptrBootSector->biosParameterBlock.fat32.sectorsPerFat32;
                    }
                    // Calculate the location of the root sector (for FAT12/16)
                    drive->firstRootSector = drive->firstFatSector + (uint32_t)(drive->fatCopyCount * (uint32_t)drive->fatSectorCount);
                    // Determine the max size of the root (will be 0 for FAT32)
                    drive->rootDirectoryEntryCount = ptrBootSector->biosParameterBlock.fat16.rootDirectoryEntryCount;

                    // Determine the total number of sectors in the partition
                    if(ptrBootSector->biosParameterBlock.fat16.totalSectors16 != 0)
                    {
                        totalSectors = ptrBootSector->biosParameterBlock.fat16.totalSectors16;
                    }
                    else
                    {
                        totalSectors = ptrBootSector->biosParameterBlock.fat16.totalSectors32;
                    }

                    // Calculate the number of bytes in each sector
                    bytesPerSector = ptrBootSector->biosParameterBlock.fat16.bytesPerSector;
                    if( (bytesPerSector == 0) || ((bytesPerSector & 1) == 1) )
                    {
                        error = FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE;
                        break;  //break out of the do while loop
                    }

                    // Calculate the number of sectors in the root (will be 0 for FAT32)
                    rootDirectorySectors = ((ptrBootSector->biosParameterBlock.fat16.rootDirectoryEntryCount * 32) + (ptrBootSector->biosParameterBlock.fat16.bytesPerSector - 1)) / ptrBootSector->biosParameterBlock.fat16.bytesPerSector;
                    // Calculate the number of data sectors on the card
                    dataSectors = totalSectors - (drive->firstRootSector + rootDirectorySectors);
                    // Calculate the maximum number of clusters on the card
                    drive->partitionClusterCount = dataSectors / drive->sectorsPerCluster;

                    #else // PIC24/30/33

                    // Read the count of reserved sectors
                    reservedSectorCount = ReadRam16bit(drive->dataBuffer, BSI_RESRVSEC);
                    // Load the count of sectors per cluster
                    drive->sectorsPerCluster = *(drive->dataBuffer + BSI_SPC);
                    // Load the sector number of the first FAT sector
                    drive->firstFatSector = drive->firstPartitionSector + reservedSectorCount;
                    // Load the count of FAT tables
                    drive->fatCopyCount = *(drive->dataBuffer + BSI_FATCOUNT);
                    // Load the size of the FATs
                    drive->fatSectorCount = ReadRam16bit (drive->dataBuffer, BSI_SPF);
                    if(drive->fatSectorCount == 0)
                    {
                        drive->fatSectorCount = ReadRam32bit (drive->dataBuffer, BSI_FATSZ32);
                    }
                    // Calculate the location of the root sector (for FAT12/16)
                    drive->firstRootSector = drive->firstFatSector + (uint32_t)(drive->fatCopyCount * (uint32_t)drive->fatSectorCount);
                    // Determine the max size of the root (will be 0 for FAT32)
                    drive->rootDirectoryEntryCount = ReadRam16bit (drive->dataBuffer, BSI_ROOTDIRENTS);

                    // Determine the total number of sectors in the partition
                    totalSectors = ReadRam16bit (drive->dataBuffer, BSI_TOTSEC16);
                    if(totalSectors == 0 )
                    {
                        totalSectors = ReadRam32bit (drive->dataBuffer, BSI_TOTSEC32);
                    }

                    // Calculate the number of uint8_ts in each sector
                    bytesPerSector = ReadRam16bit (drive->dataBuffer, BSI_BPS);
                    if((bytesPerSector == 0) || ((bytesPerSector & 1) == 1))
                    {
                        error = FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE;
                        break;
                    }

                    // Calculate the number of sectors in the root (will be 0 for FAT32)
                    rootDirectorySectors = ((drive->rootDirectoryEntryCount * FILEIO_DIRECTORY_ENTRY_SIZE) + (bytesPerSector - 1)) / bytesPerSector;
                    // Calculate the number of data sectors on the card
                    dataSectors = totalSectors - (reservedSectorCount + (drive->fatCopyCount * drive->fatSectorCount)  + rootDirectorySectors);
                    // Calculate the maximum number of clusters on the card
                    drive->partitionClusterCount = dataSectors / drive->sectorsPerCluster;

                    #endif

                    // Determine the file system type based on the number of clusters used
                    if(drive->partitionClusterCount < 4085)
                    {
                        drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT12;
                    }
                    else
                    {
                        if(drive->partitionClusterCount < 65525)
                        {
                            drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT16;
                        }
                        else
                        {
                            drive->type = FILEIO_FILE_SYSTEM_TYPE_FAT32;
                        }
                    }

                    if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)
                    {
                        #ifdef __XC8__
                            drive->firstRootCluster = ptrBootSector->biosParameterBlock.fat32.firstClusterRootDirectory;
                        #else
                            drive->firstRootCluster = ReadRam32bit (drive->dataBuffer, BSI_ROOTCLUS);
                        #endif
                        drive->firstDataSector = drive->firstRootSector + rootDirectorySectors;
                    }
                    else
                    {
                        drive->firstRootCluster = 0;
                        drive->firstDataSector = drive->firstRootSector + (drive->rootDirectoryEntryCount >> 4);
                    }

                    if(bytesPerSector > FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
                    {
                        error = FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE;
                    }

                }while(0);  // do/while loop designed to allow to break out if
                            //   there is an error detected without returning
                            //   from the function.

            }

            if ((drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT32) || ((error != FILEIO_ERROR_NONE) &&
                ((ptrBootSector->biosParameterBlock.fat32.bootSignature == 0x29) ||
                (ptrBootSector->biosParameterBlock.fat32.bootSignature == 0x28))))
            {
                //Check for possible errors in the formatting
                if((ptrBootSector->biosParameterBlock.fat32.totalSectors16 != 0)
                    || ((ptrBootSector->biosParameterBlock.fat32.bootSignature != 0x29) &&
                        (ptrBootSector->biosParameterBlock.fat32.bootSignature != 0x28))
                  )
                {
                    error = FILEIO_ERROR_NOT_FORMATTED;
                }

                //If there were formatting errors then in FAT32 we can try to use
                //  the backup boot sector
                if((error != FILEIO_ERROR_NONE) && (triedSpecifiedBackupBootSec == false))
                {
                    triedSpecifiedBackupBootSec = true;

                    if ((*drive->driveConfig->funcSectorRead) (drive->mediaParameters, drive->firstPartitionSector + ptrBootSector->biosParameterBlock.fat32.backupBootSector, drive->dataBuffer) != true)
                    {
                        error = FILEIO_ERROR_BAD_SECTOR_READ;
                        break;
                    }
                    else
                    {
                        error = FILEIO_ERROR_NONE;
                        continue;
                    }
                }

                if((error != FILEIO_ERROR_NONE) && (triedBackupBootSecAtAddress6 == false))
                {
                    triedBackupBootSecAtAddress6 = true;

                    //Here we are using the magic number 6 because the FAT32 specification
                    //  recommends that "No value other than 6 is recommended."  We've
                    //  already tried using the value specified in the BPB_BkBootSec
                    //  field and it must have failed
                    if ((*drive->driveConfig->funcSectorRead) (drive->mediaParameters, drive->firstPartitionSector + 6, drive->dataBuffer) != true)
                    {
                        error = FILEIO_ERROR_BAD_SECTOR_READ;
                        break;
                    }
                    else
                    {
                        error = FILEIO_ERROR_NONE;
                        continue;
                    }
                }

            }   //type == FAT32
            break;
        }
        while(1);
    }

    return error;
}

int FILEIO_DriveUnmount (const uint16_t driveId)
{
    FILEIO_DRIVE * drive;
    uint8_t i;

    drive = FILEIO_CharToDrive (driveId);

    if (drive == NULL)
    {
        return FILEIO_RESULT_FAILURE;
    }
    else
    {
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    #if !defined (FILEIO_CONFIG_WRITE_DISABLE)
        if (drive->bufferStatusPtr->driveOwner == drive)
        {
            FILEIO_FlushBuffer (drive, FILEIO_BUFFER_FAT);
            FILEIO_FlushBuffer (drive, FILEIO_BUFFER_DATA);
            drive->bufferStatusPtr->driveOwner = NULL;
        }
    #endif
#else
    #if !defined (FILEIO_CONFIG_WRITE_DISABLE)
        FILEIO_FlushBuffer (drive, FILEIO_BUFFER_FAT);
        FILEIO_FlushBuffer (drive, FILEIO_BUFFER_DATA);
    #endif
#endif
    }

    drive->driveConfig->funcMediaDeinit(drive->mediaParameters);

    for (i = 0; i < FILEIO_CONFIG_MAX_DRIVES; i++)
    {
        if (gDriveArray[i].driveId == driveId)
        {
            gDriveSlotOpen[i] = true;
            gDriveArray[i].mount = false;
            gDriveArray[i].driveId = 0;
            break;
        }
    }


    if (globalParameters.currentWorkingDirectory.drive == drive)
    {
        globalParameters.currentWorkingDirectory.cluster = 0;
        globalParameters.currentWorkingDirectory.drive = NULL;
    }

    return FILEIO_RESULT_SUCCESS;
}

const uint16_t gShortFileNameCharacters[17] =
{
    '!', '#', '$', '%', '&', '\'', '(', ')', '-', '@', '^', '_', '`', '{', '}', '~', ' '
};

const uint16_t gLongFileNameCharacters[9] =
{
    '\\', '/', ':', '*', '?', '"', '<', '>', '|'
};


uint8_t FILEIO_FileNameTypeGet(const uint16_t * fileName, bool partialStringSearch)
{
    uint16_t c;
    uint8_t i;
    FILEIO_NAME_TYPE type = FILEIO_NAME_SHORT;
    bool foundRadix = false;
    uint8_t count = 0;

    if (*fileName == '.')
    {
        c = *(fileName + 1);
        if ((c == 0) || (c == FILEIO_CONFIG_DELIMITER))
        {
            return FILEIO_NAME_DOT;
        }
        if (c == '.')
        {
            c = *(fileName + 2);
            if ((c == 0) || (c == FILEIO_CONFIG_DELIMITER))
            {
                return FILEIO_NAME_DOT;
            }
        }
    }

    // Iterate though the whole file name
    while (((c = (uint8_t)*fileName++) != 0) && (c != FILEIO_CONFIG_DELIMITER))
    {
        if (type == FILEIO_NAME_SHORT)
        {
            count++;
            if (!(((c >= 'A') && (c <= 'Z')) ||
                ((c >= '0') && (c <= '9')) ||
                (c >= 128)))
            {
                if (c == '.')
                {
                    if (!foundRadix)
                    {
                        foundRadix = true;
                        count = 0;
                    }
                    else
                    {
                        type = FILEIO_NAME_LONG;
                    }
                }
                else
                {
                    // C could still be a special character
                    for (i = 0; i < sizeof (gShortFileNameCharacters) / 2; i++)
                    {
                        if (c == gShortFileNameCharacters[i])
                        {
                            break;
                        }
                    }
                    if (i == sizeof (gShortFileNameCharacters) / 2)
                    {
                        if (!partialStringSearch || ((c != '?') && (c != '*')))
                        {
                            // Switch the type to long
                            type = FILEIO_NAME_LONG;
                        }
                    }
                }
            }
            if (foundRadix)
            {
                if (count > 3)
                {
                    type = FILEIO_NAME_LONG;
                }
            }
            else
            {
                if (count > 8)
                {
                    type = FILEIO_NAME_LONG;
                }
            }
        }
        if (type == FILEIO_NAME_LONG)
        {
            for (i = 0; i < sizeof (gLongFileNameCharacters) / 2; i++)
            {
                if (c == gLongFileNameCharacters[i])
                {
                    if (!partialStringSearch || ((c != '?') && (c != '*')))
                    {
                        type = FILEIO_NAME_INVALID;
                        return type;
                    }
                }
            }
        }
    }
    return type;
}

void FILEIO_FormatShortFileName (const uint16_t * fileName, FILEIO_OBJECT * filePtr)
{
    uint16_t c;
    uint8_t i = 0;

    while (((c = *fileName++) != '.') && (c != 0) && (c != FILEIO_CONFIG_DELIMITER))
    {
        filePtr->name[i++] = c;
    }

    while (i < 8)
    {
        filePtr->name[i++] = 0x20;
    }

    if (c == '.')
    {
        while ((c = *fileName++) != 0)
        {
            filePtr->name[i++] = c;
        }
    }

    while (i < 11)
    {
        filePtr->name[i++] = 0x20;
    }

    filePtr->lfnPtr = NULL;
}


int FILEIO_Open (FILEIO_OBJECT * filePtr, const uint16_t * fileName, uint16_t mode)
{
    FILEIO_ERROR_TYPE error;
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
    uint16_t entryHandle;
#endif
    FILEIO_DIRECTORY directory;
    uint8_t fileNameType;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;

    fileName = FILEIO_CacheDirectory (&directory, (uint16_t *)fileName, false);

    if (fileName == NULL)
    {
        return FILEIO_RESULT_FAILURE;
    }
    
    currentCluster = directory.cluster;

#if defined (FILEIO_CONFIG_WRITE_DISABLE)
    if (mode & (FILEIO_OPEN_WRITE | FILEIO_OPEN_CREATE | FILEIO_OPEN_TRUNCATE))
    {
        directory.drive->error = FILEIO_ERROR_WRITE_PROTECTED;
        return FILEIO_RESULT_FAILURE;
    }
#endif

    if((*directory.drive->driveConfig->funcWriteProtectGet)(directory.drive->mediaParameters) && ((mode & (FILEIO_OPEN_WRITE | FILEIO_OPEN_CREATE | FILEIO_OPEN_TRUNCATE)) != 0))
    {
        directory.drive->error = FILEIO_ERROR_WRITE_PROTECTED;
        return FILEIO_RESULT_FAILURE;
    }

    // Check to ensure that a file object was allocated
    if (filePtr == NULL)
    {
        directory.drive->error = FILEIO_ERROR_TOO_MANY_FILES_OPEN;
        return FILEIO_RESULT_FAILURE;
    }

    fileNameType = FILEIO_FileNameTypeGet(fileName, false);

    if (fileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (fileName, filePtr);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (&directory, filePtr, (uint8_t *)filePtr->name, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (fileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        filePtr->lfnPtr = (uint16_t *)fileName;
        filePtr->lfnLen = FILEIO_strlen16 ((uint16_t *)fileName);
        error = FILEIO_FindLongFileName (&directory, filePtr, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else
    {
        directory.drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
    if (error == FILEIO_ERROR_NONE)
    {
        // File was found
        if (((mode & FILEIO_OPEN_TRUNCATE) == FILEIO_OPEN_TRUNCATE) || !FILEIO_IsClusterAllocated(&directory, filePtr))
        {
            entryHandle = filePtr->entry;
            error = FILEIO_EraseFile (filePtr, &entryHandle, true);
            mode |= FILEIO_OPEN_CREATE;
        }
        else
        {
            // Use the CREATE flag to indicate whether the file should be created
            mode &= ~FILEIO_OPEN_CREATE;
        }
    }
    else
    {
        // We couldn't find the file
        if (error == FILEIO_ERROR_DONE)
        {
            // File wasn't found
            if ((mode & FILEIO_OPEN_CREATE) == FILEIO_OPEN_CREATE)
            {
                error = FILEIO_ERROR_NONE;
                filePtr->disk = directory.drive;
                filePtr->baseClusterDir = directory.cluster;
                filePtr->currentClusterDir = directory.cluster;
            }
        }
    }

    if (error == FILEIO_ERROR_NONE)
    {
        if ((mode & FILEIO_OPEN_CREATE) == FILEIO_OPEN_CREATE)
        {
            // Try to create the file (or replace it, if the file was truncated
            entryHandle = 0;

            error = FILEIO_DirectoryEntryCreate (filePtr, &entryHandle, 0, true);

            mode |= FILEIO_OPEN_APPEND;
        }
    }
#endif

    if (error == FILEIO_ERROR_NONE)
    {
        if ((mode & FILEIO_OPEN_READ) == FILEIO_OPEN_READ)
        {
            filePtr->flags.readEnabled = true;
        }
        else
        {
            filePtr->flags.readEnabled = false;
        }

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
        if ((mode & FILEIO_OPEN_WRITE) == FILEIO_OPEN_WRITE)
        {
            filePtr->flags.writeEnabled = true;
        }
        else
        {
            filePtr->flags.writeEnabled = false;
        }
#endif

        if ((mode & FILEIO_OPEN_APPEND) == FILEIO_OPEN_APPEND)
        {
            int result = FILEIO_Seek (filePtr, 0, FILEIO_SEEK_END);
            if (result != FILEIO_RESULT_SUCCESS)
            {
                error = FILEIO_ERROR_SEEK_ERROR;
            }
            else
            {
                mode &= ~FILEIO_OPEN_APPEND;
            }
        }
    }

    // Check to ensure no errors occured
    if (error != FILEIO_ERROR_NONE)
    {
        directory.drive->error = error;
        return FILEIO_RESULT_FAILURE;
    }

    return FILEIO_RESULT_SUCCESS;
}

bool FILEIO_IsClusterAllocated(FILEIO_DIRECTORY * directory, FILEIO_OBJECT * filePtr)
{
    FILEIO_ERROR_TYPE error;
    uint32_t currentCluster = filePtr->baseClusterDir;
    uint16_t currentClusterOffset = 0;
    FILEIO_DIRECTORY_ENTRY * entry;

    if (currentCluster == 0)
    {
        currentCluster = directory->drive->firstRootCluster;
    }

    entry = FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, filePtr->entry);

    if (entry == NULL)
    {
        return false;
    }

    if ((entry->firstClusterHigh == 0) && (entry->firstClusterLow == 0))
    {
        return false;
    }

    return true;
}

uint32_t FILEIO_FullClusterNumberGet (FILEIO_DIRECTORY_ENTRY * entry)
{
    uint32_t result;

    result = entry->firstClusterHigh;
    result <<= 16;
    result |= entry->firstClusterLow;

    return result;
}

uint16_t * FILEIO_CacheDirectory (FILEIO_DIRECTORY * dir, uint16_t * path, bool createDirectories)
{
    uint16_t pathLen;
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
    uint16_t i;
#endif

    pathLen = FILEIO_strlen16 ((uint16_t *)path);

    if (pathLen == 0)
    {
        return NULL;
    }
    
    memcpy (dir, &globalParameters.currentWorkingDirectory, sizeof (FILEIO_DIRECTORY));

#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
    if (pathLen == 1)
    {
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        if (FILEIO_GetSingleBuffer (dir->drive) != FILEIO_RESULT_SUCCESS)
        {
            return NULL;
        }
#endif
        return path;
    }

    // Check to see if the user explicitly specified the drive
    if (*(path + 1) == ':')
    {
        // They did
        dir->drive = FILEIO_CharToDrive (*path);
        if (dir->drive == NULL)
        {
            return NULL;
        }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        if (FILEIO_GetSingleBuffer (dir->drive) != FILEIO_RESULT_SUCCESS)
        {
            return NULL;
        }
#endif

        dir->cluster = dir->drive->firstRootCluster;

        // Increment past the drive specifier
        path += 2;
        pathLen -= 2;
        if (pathLen == 0)
        {
            return path;
        }

        if (*path == FILEIO_CONFIG_DELIMITER)
        {
            path++;
            pathLen--;
            if (pathLen == 0)
            {
                return path;
            }
        }
    }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (dir->drive) != FILEIO_RESULT_SUCCESS)
    {
        return NULL;
    }
#endif

    // Find the next forward slash (indicates part of the path is a directory)
    while ((i = FILEIO_FindNextDelimiter(path)) != -1)
    {
        // If someone terminated a directory path with a delimiter, break out of the loop
        if (*(path + i) == FILEIO_CONFIG_DELIMITER)
        {
            if (*(path + i + 1) == 0)
            {
                break;
            }
        }
        
        // If i == 0, someone put two delimiters in a row or something strange; don't change directory
        if (i != 0)
        {
            // Try to change dir to that directory
            if (FILEIO_DirectoryChangeSingle (dir, path) != FILEIO_RESULT_SUCCESS)
            {
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
                if (createDirectories)
                {
                    // If we couldn't change to that directory, it probably doesn't exist.  Try to create it.
                    if (FILEIO_DirectoryMakeSingle (dir, path) != FILEIO_RESULT_SUCCESS)
                    {
                        return NULL;
                    }
                    // Now that it exists, try to change to it.
                    if (FILEIO_DirectoryChangeSingle (dir, path) != FILEIO_RESULT_SUCCESS)
                    {
                        return NULL;
                    }
                }
                else
#endif
                {
                    return NULL;
                }
            }
        }

        // Increment i to include the delimiter
        i++;
        // Increment the path
        path += i;
        // Decrement the path length
        pathLen -= i;
    }
#endif

    // Whatever is left must be our file name.  dir will contain the drive and cluster of the directory.
    return path;
}

#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
uint16_t FILEIO_FindNextDelimiter(const uint16_t * path)
{
    uint16_t i = 0;
    uint16_t c;

    while (((c = *path++) != 0) && (c != FILEIO_CONFIG_DELIMITER))
    {
        i++;
    }

    if (c == 0)
    {
        return -1;
    }
    else
    {
        return i;
    }
}
#endif

#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
FILEIO_RESULT FILEIO_DirectoryChangeSingle (FILEIO_DIRECTORY * directory, uint16_t * path)
{
    FILEIO_ERROR_TYPE error;
    uint8_t fileNameType;
    uint32_t currentCluster;
    uint16_t currentClusterOffset;
    FILEIO_OBJECT file;
    FILEIO_OBJECT * filePtr = &file;

    fileNameType = FILEIO_FileNameTypeGet(path, false);

    if (fileNameType == FILEIO_NAME_INVALID)
    {
		directory->drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (fileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory->cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (path, filePtr);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (directory, filePtr, (uint8_t *)filePtr->name, &currentCluster, &currentClusterOffset, 0, FILEIO_ATTRIBUTE_MASK, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (fileNameType == FILEIO_NAME_DOT)
    {
        // If someone specified a '.' filename, we don't have to change anything
        if (*(path + 1) != '.')
        {
            return FILEIO_RESULT_SUCCESS;
        }

        // If they specified a dotdot filename, cache the previous directory's cluster
        {
            FILEIO_DIRECTORY_ENTRY * entry;

            currentCluster = directory->cluster;
            currentClusterOffset = 0;

            // Cache the .. entry
            entry = FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, 1);
            if (error == FILEIO_ERROR_NONE)
            {
                directory->cluster = FILEIO_FullClusterNumberGet (entry);
                return FILEIO_RESULT_SUCCESS;
            }
            else
            {
                return FILEIO_RESULT_FAILURE;
            }
        }
    }
    else
    {
        // Long file name
        currentCluster = directory->cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        filePtr->lfnPtr = path;
        filePtr->lfnLen = FILEIO_lfnlen(path);
        error = FILEIO_FindLongFileName (directory, filePtr, &currentCluster, &currentClusterOffset, 0, FILEIO_ATTRIBUTE_MASK, FILEIO_SEARCH_ENTRY_MATCH);
    }

    if (error == FILEIO_ERROR_NONE)
    {
        // Directory found
        directory->cluster = filePtr->firstCluster;
    }
    else
    {
        return FILEIO_RESULT_FAILURE;
    }

    return FILEIO_RESULT_SUCCESS;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
FILEIO_RESULT FILEIO_DirectoryMakeSingle (FILEIO_DIRECTORY * directory, uint16_t * path)
{
    FILEIO_ERROR_TYPE error;
    FILEIO_OBJECT file;
    FILEIO_OBJECT * filePtr = &file;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_TIMESTAMP timeStamp;
    uint16_t entryOffset = 0;
    uint32_t dot, dotdot;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    uint8_t fileNameType;

    file.baseClusterDir = directory->cluster;
    if (file.baseClusterDir == 0)
    {
        file.baseClusterDir = directory->drive->firstRootCluster;
    }
    file.disk = directory->drive;

    fileNameType = FILEIO_FileNameTypeGet(path, false);

    if ((fileNameType == FILEIO_NAME_INVALID) || (fileNameType == FILEIO_NAME_DOT))
    {
        directory->drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (fileNameType == FILEIO_NAME_SHORT)
    {
        // Short file name
        FILEIO_FormatShortFileName (path, filePtr);
    }
    else
    {
        filePtr->lfnPtr = (uint16_t *)path;
        filePtr->lfnLen = FILEIO_lfnlen ((uint16_t *)path);
    }

    if (FILEIO_DirectoryEntryCreate (filePtr, &entryOffset, FILEIO_ATTRIBUTE_DIRECTORY, true) != FILEIO_ERROR_NONE)
    {
        return FILEIO_RESULT_FAILURE;
    }

    if (file.baseClusterDir == ((FILEIO_DRIVE *)file.disk)->firstRootCluster)
    {
        dotdot = 0;
    }
    else
    {
        dotdot = file.baseClusterDir;
    }

    file.currentClusterDir = file.baseClusterDir;
    directory->cluster = file.baseClusterDir;
    currentCluster = file.baseClusterDir;

    entry = FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, entryOffset);

    if (entry == NULL)
    {
        directory->drive->error = error;
        return FILEIO_RESULT_FAILURE;
    }

    timeStamp.timeMs = entry->createTimeMs;
    timeStamp.time.value = entry->createTime;
    timeStamp.date.value = entry->createDate;

    dot = FILEIO_FullClusterNumberGet (entry);

    directory->cluster = file.firstCluster;
    currentCluster = file.firstCluster;
    currentClusterOffset = 0;
    entryOffset = 0;

    entry = FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, entryOffset);

    directory->cluster = file.baseClusterDir;
    
    if (FILEIO_DotEntryWrite(directory->drive, dot, dotdot, &timeStamp))
    {
        return FILEIO_RESULT_SUCCESS;
    }
    else
    {
        return FILEIO_RESULT_FAILURE;
    }
}
#endif
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
int FILEIO_DotEntryWrite (FILEIO_DRIVE * drive, uint32_t dot, uint32_t dotdot, FILEIO_TIMESTAMP * timeStamp)
{
    FILEIO_DIRECTORY_ENTRY * entryPtr;
    uint32_t sector;

	memset(drive->dataBuffer, 0x00, drive->sectorSize);

    entryPtr = (FILEIO_DIRECTORY_ENTRY *)drive->dataBuffer;

    memset (drive->dataBuffer, 0x20, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);
    entryPtr->name[0] = '.';
    entryPtr->attributes = FILEIO_ATTRIBUTE_DIRECTORY;
    entryPtr->reserved0 = 0x00;
    entryPtr->firstClusterLow = (uint16_t)(dot & 0x0000FFFF); // Lower 16 bit address
    entryPtr->firstClusterHigh = (uint16_t)((dot & 0x0FFF0000)>> 16); // Higher 16 bit address. FAT32 uses only 28 bits. Mask even higher nibble also.

    entryPtr->fileSize = 0x00;

    // Times need to be the same as the times in the directory entry

    entryPtr->createTimeMs = timeStamp->timeMs;         // millisecond stamp
    entryPtr->createTime = timeStamp->time.value;      // time created //
    entryPtr->createDate = timeStamp->time.value;      // date created (1/1/2004)
    entryPtr->accessDate =   0x0000;         // Last Access date
    entryPtr->writeTime =      0x0000;         // last update time
    entryPtr->writeDate =      0x0000;         // last update date

    memcpy (drive->dataBuffer + sizeof (FILEIO_DIRECTORY_ENTRY), drive->dataBuffer, sizeof (FILEIO_DIRECTORY_ENTRY));
    entryPtr = (FILEIO_DIRECTORY_ENTRY *)(drive->dataBuffer + sizeof (FILEIO_DIRECTORY_ENTRY));
    entryPtr->name[1] = '.';

    entryPtr->firstClusterLow = (uint16_t)(dotdot & 0x0000FFFF); // Lower 16 bit address
    entryPtr->firstClusterHigh = (uint16_t)((dotdot & 0x0FFF0000)>> 16); // Higher 16 bit address. FAT32 uses only 28 bits. Mask even higher nibble also.

    sector = FILEIO_ClusterToSector (drive, dot);

    drive->bufferStatusPtr->flags.dataBufferNeedsWrite = true;
    drive->bufferStatusPtr->dataBufferCachedSector = sector;

    if (!FILEIO_FlushBuffer (drive, FILEIO_BUFFER_DATA))
    {
        return false;
    }

    return true;
}
#endif
#endif

FILEIO_ERROR_TYPE FILEIO_FindShortFileName (FILEIO_DIRECTORY * directory, FILEIO_OBJECT * filePtr, uint8_t * fileName, uint32_t * currentCluster, uint16_t * currentClusterOffset, uint16_t entryOffset, uint16_t attributes, FILEIO_SEARCH_TYPE mode)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DIRECTORY_ENTRY * entry;

    if (*currentCluster == 0)
    {
        *currentCluster = directory->drive->firstRootCluster;
    }

    while(1)
    {
        do
        {
            entry = FILEIO_DirectoryEntryCache (directory, &error, currentCluster, currentClusterOffset, entryOffset);
            if (error == FILEIO_ERROR_DONE)
            {
                break;
            }
            else if (error == FILEIO_ERROR_BAD_CACHE_READ)
            {
                directory->drive->error = FILEIO_ERROR_BAD_CACHE_READ;
                return error;
            }

            entryOffset++;
        } while (((entry->attributes == FILEIO_ATTRIBUTE_LONG_NAME) || (entry->attributes == FILEIO_ATTRIBUTE_VOLUME) || (((uint8_t)entry->name[0]) == FILEIO_DIRECTORY_ENTRY_DELETED)) && (entry->name[0] != FILEIO_DIRECTORY_ENTRY_EMPTY));

        if ((error == FILEIO_ERROR_DONE) || (entry->name[0] == FILEIO_DIRECTORY_ENTRY_EMPTY))
        {
            return FILEIO_ERROR_DONE;
        }

        // Valid file entry was found
        if (((mode & FILEIO_SEARCH_ENTRY_ATTRIBUTES) != FILEIO_SEARCH_ENTRY_ATTRIBUTES) || ((entry->attributes & attributes) == entry->attributes))
        {
            // File's attributes are valid or we aren't trying to match attributes
            if (FILEIO_ShortFileNameCompare (fileName, (uint8_t *)entry->name, mode) == true)
            {
                // Found a match.  Fill the result object with the file data
                memcpy (filePtr->name, entry->name, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);
                filePtr->disk = directory->drive;
                filePtr->firstCluster = FILEIO_FullClusterNumberGet (entry);
                filePtr->currentCluster = filePtr->firstCluster;
                filePtr->currentSector = 0;
                filePtr->currentOffset = 0;
                filePtr->absoluteOffset = 0;
                filePtr->size = entry->fileSize;

                filePtr->attributes = entry->attributes;
                if ((filePtr->attributes & FILEIO_ATTRIBUTE_DIRECTORY) == FILEIO_ATTRIBUTE_DIRECTORY)
                {
                    filePtr->timeMs = entry->createTimeMs;
                    filePtr->time = entry->createTime;
                    filePtr->date = entry->createDate;
                }
                else
                {
                    filePtr->time = entry->writeTime;
                    filePtr->date = entry->writeDate;
                }
                filePtr->entry = --entryOffset;
                filePtr->baseClusterDir = directory->cluster;
                filePtr->currentClusterDir = directory->cluster;

                return FILEIO_ERROR_NONE;
            }
        }
    }

#if defined (__XC8__)
    // This return value is not reachable, but it is necessary to remove an XC8 compiler warning
    return -1;
#endif
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryCreate (FILEIO_OBJECT * filePtr, uint16_t * entryHandle, uint8_t attributes, bool allocateDataCluster)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    uint32_t cluster;

    *entryHandle = 0;

    if (FILEIO_DirectoryEntryFindEmpty(filePtr, entryHandle) == FILEIO_ERROR_NONE)
    {
        // Allocate a data cluster to the file object, if necessary
        if (allocateDataCluster)
        {
            cluster = FILEIO_CreateFirstCluster (filePtr);
            error = ((FILEIO_DRIVE *)filePtr->disk)->error;
        }
        else
        {
            cluster = 0x0000000;
        }

        // Construct and populate the long file name entries
        if (error == FILEIO_ERROR_NONE)
        {
            if (filePtr->lfnPtr != NULL)
            {
                error = FILEIO_DirectoryEntryLFNCreate (filePtr, entryHandle);
            }
        }

        // Construct and populate the short file entry
        if (error == FILEIO_ERROR_NONE)
        {
            error = FILEIO_DirectoryEntryPopulate(filePtr, entryHandle, attributes, cluster);
        }
    }
    else
    {
        error = FILEIO_ERROR_DIR_FULL;
    }

    ((FILEIO_DRIVE *)filePtr->disk)->error = error;
    return error;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
uint32_t FILEIO_CreateFirstCluster (FILEIO_OBJECT * filePtr)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DRIVE * drive = filePtr->disk;
    uint32_t cluster;

    cluster = FILEIO_FindEmptyCluster (filePtr->disk, 2);

    if (cluster == 0)
    {
        error = FILEIO_ERROR_DRIVE_FULL;
    }
    else
    {
                // mark the cluster as taken, and last in chain
        if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT12)
        {
            if(FILEIO_FATWrite (drive, cluster, FILEIO_CLUSTER_VALUE_FAT12_EOF, false) == FILEIO_CLUSTER_VALUE_FAT16_FAIL)
            {
                error = FILEIO_ERROR_WRITE;
            }
        }
        else if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
        {
            if(FILEIO_FATWrite (drive, cluster, FILEIO_CLUSTER_VALUE_FAT16_EOF, false) == FILEIO_CLUSTER_VALUE_FAT16_FAIL)
            {
                error = FILEIO_ERROR_WRITE;
            }
        }
        else
        {
            if(FILEIO_FATWrite (drive, cluster, FILEIO_CLUSTER_VALUE_FAT32_EOF, false) == FILEIO_CLUSTER_VALUE_FAT32_FAIL)
            {
                error = FILEIO_ERROR_WRITE;
            }
        }

        // lets erase this cluster
        if(error == FILEIO_ERROR_NONE)
        {
            error = FILEIO_EraseCluster(drive, cluster);
        }
    }

    if (!FILEIO_FlushBuffer (drive, FILEIO_BUFFER_FAT))
    {
        error = FILEIO_ERROR_WRITE;
    }

    filePtr->firstCluster = cluster;
    filePtr->currentCluster = cluster;

    drive->error = error;

    return cluster;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryLFNCreate (FILEIO_OBJECT * filePtr, uint16_t * entryHandle)
{
    char * source;
    uint8_t checksum = 0;
    uint8_t i;
    uint8_t remainder, fileEntryCount, sequenceNumber;
    uint16_t length;
    uint16_t * tempNamePtr;
    bool firstLoop = true;
    FILEIO_DIRECTORY_ENTRY_LFN * lfnEntry;
    FILEIO_DIRECTORY directory;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    uint32_t currentCluster;
    uint16_t currentClusterOffset;

    if (!FILEIO_AliasLFN(filePtr))
    {
        return FILEIO_ERROR_FILENAME_EXISTS;
    }

    source = filePtr->name;

    // Calculate the short file name checksum for this file
    for (i = 11; i != 0; i--)
    {
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *source++;
    }

    length = filePtr->lfnLen;

    // Calculate the number of entries required for the given file name
    remainder = length % FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;
    fileEntryCount = length / FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;

    if (remainder || (length < (FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY)))
    {
        fileEntryCount++;
        tempNamePtr = filePtr->lfnPtr + length - remainder;
    }
    else
    {
        tempNamePtr = filePtr->lfnPtr + length - FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;
    }

    if (remainder == 0)
    {
        remainder = FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;
    }

    // Calculate maximum sequence number for LFN root entries
    sequenceNumber = fileEntryCount | 0x40;

    directory.cluster = filePtr->baseClusterDir;
    directory.drive = filePtr->disk;
    currentCluster = directory.cluster;
    currentClusterOffset = 0;

    while (fileEntryCount)
    {
        lfnEntry = (FILEIO_DIRECTORY_ENTRY_LFN *)FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, *entryHandle);
        
        if (lfnEntry == NULL)
        {
            return FILEIO_ERROR_BAD_CACHE_READ;
        }
        
        ((FILEIO_DRIVE *)filePtr->disk)->bufferStatusPtr->flags.dataBufferNeedsWrite = true;

        lfnEntry->sequenceNumber = sequenceNumber--;
        lfnEntry->attributes = FILEIO_ATTRIBUTE_LONG_NAME;
        lfnEntry->type = 0;
        lfnEntry->checksum = checksum;
        lfnEntry->reserved0 = 0;

        // Copy the name
        if (firstLoop)
        {
            uint16_t tempArray[FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY];

            memcpy (tempArray, tempNamePtr, remainder << 1);
            // If this name isn't an even multiple of FILEIO_NAME_NAME_UTF16_CHARS_IN_LFN_ENTRY, we need to null-terminate it
            if (remainder != FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY)
            {
                tempArray[remainder++] = 0x0000;
            }

            memset (tempArray + remainder, 0xFF, (FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY - remainder) << 1);

            memcpy (&lfnEntry->namePart1, tempArray, 10);
            memcpy (&lfnEntry->namePart2, &tempArray[5], 12);
            memcpy (&lfnEntry->namePart3, &tempArray[11], 4);

            // Decrement tempNamePtr to the beginning of the next file entry
            tempNamePtr -= FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;

            firstLoop = false;
            sequenceNumber &= (~0x40);
        }
        else
        {
            memcpy (&lfnEntry->namePart1, tempNamePtr, 10);
            memcpy (&lfnEntry->namePart2, tempNamePtr + 5, 12);
            memcpy (&lfnEntry->namePart3, tempNamePtr + 11, 4);
            // Decrement tempNamePtr to the beginning of the next file entry
            tempNamePtr -= FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY;
        }

        *entryHandle = *entryHandle + 1;
        fileEntryCount--;
    }

    // Store the next entry (the short file name entry) in the file pointer's entry field
    filePtr->entry = *entryHandle;
    
    // Don't bother to force a write; the entries will get updated when the short file name entry is written
    return error;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
bool FILEIO_AliasLFN (FILEIO_OBJECT * filePtr)
{
    uint32_t loopIndex, loopIndexEnd;
    int16_t   index1, extIndex, lfnIndex;
    uint8_t  i, j;
    uint8_t tempString[FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX];
    uint16_t * templfnPtr;
    uint16_t length;
    FILEIO_DIRECTORY directory;
    uint32_t currentCluster;
    uint16_t currentClusterOffset;
    bool forceTail = false;
    uint8_t tailLen = 0;

    templfnPtr = filePtr->lfnPtr;
    length = filePtr->lfnLen;

    // Initially fill the alias name with space characters
    memset (filePtr->name, ' ', FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);

    // find the location where '.' is present
    for(lfnIndex = length - 1; lfnIndex > 0; lfnIndex--)
    {
        if (templfnPtr[lfnIndex] == '.')
        {
            break;
        }
    }

    index1 = lfnIndex + 1;
    if(lfnIndex)
    {
        // Complete the extension part as per the FAT specifications
        for(extIndex = 8, j = 0; (index1 < length) && (j < 3); index1++)
        {
            // Convert lower-case to upper-case
            i = (uint8_t)templfnPtr[index1];
            if ((i >= 0x61) && (i <= 0x7A))
            {
                filePtr->name[extIndex++] = i - 0x20;
            }
            else if (i == ' ')
            {
                continue;
            }
            else if ((i == 0x2B) || (i == 0x2C) || (i == 0x3B) ||
                (i == 0x3D) || (i == 0x5B) || (i == 0x5D) ||
                (templfnPtr[index1] > 0xFF))
            {
                filePtr->name[extIndex++] = '_';
                forceTail = true;
            }
            else if (i == FILEIO_CONFIG_DELIMITER)
            {
                break;
            }
            else
            {
                filePtr->name[extIndex++] = i;
            }

            j++;
        }

        extIndex = lfnIndex;
    }
    else
    {
        extIndex = length;
    }

    // Fill the base part as per the FAT specifications
    for(index1 = 0, j = 0; ((index1 < extIndex) && (j < 8)); index1++)
    {
        // Convert lower-case to upper-case
        i = (uint8_t)templfnPtr[index1];
        if ((i >= 0x61) && (i <= 0x7A))
        {
            filePtr->name[j] = i - 0x20;
        }
        else if ((i == ' ') || (i == '.'))
        {
            continue;
        }
        else if ((i == 0x2B) || (i == 0x2C) || (i == 0x3B) ||
            (i == 0x3D) || (i == 0x5B) || (i == 0x5D) ||
            (templfnPtr[index1] > 0xFF))
        {
            filePtr->name[j] = '_';
            forceTail = true;
        }
        else
        {
            filePtr->name[j] = i;
        }

        j++;
    }

    filePtr->attributes = FILEIO_ATTRIBUTE_ARCHIVE;

    // Check to see if we need to append a numeric index to the name

    directory.cluster = filePtr->baseClusterDir;
    directory.drive = filePtr->disk;
    currentCluster = directory.cluster;
    currentClusterOffset = 0;

    // See if the current short file name exists or if we need to force a tail because our LFN contained unconvertable unicode characters
    if (forceTail || (FILEIO_FindShortFileName (&directory, filePtr, (uint8_t *)&filePtr->name, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH) == FILEIO_ERROR_NONE))
    {
        memcpy (tempString, &filePtr->name, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);

        // The max number of name characters with a numeric index is 6
        if (j > 6)
        {
            j = 6;
        }

        loopIndex = 1;
        
        // Outer loop - Adjust the length of the tail and add the '~' character
        for ( ; j > 0; j--)
        {
            tempString[j] = '~';
            switch (j)
            {
                case 6:
                    tailLen = 2;
                    loopIndexEnd = 10;
                    break;
                case 5:
                    tailLen = 3;
                    loopIndexEnd = 100;
                    break;
                case 4:
                    tailLen = 4;
                    loopIndexEnd = 1000;
                    break;
                case 3:
                    tailLen = 5;
                    loopIndexEnd = 10000;
                    break;
                case 2:
                    tailLen = 6;
                    loopIndexEnd = 100000;
                    break;
                case 1:
                    tailLen = 7;
                    loopIndexEnd = 1000000;
                    break;
            }

            // Inner loop - increase the value of the numeric index until we find one that isn't taken
            for ( ; loopIndex < loopIndexEnd; loopIndex++)
            {
                currentCluster = directory.cluster;
                currentClusterOffset = 0;

                switch (tailLen)
                {
                    case 7:
                        tempString[2] = (loopIndex % 1000000)/100000 + '0';
                    case 6:
                        tempString[3] = (loopIndex % 100000)/10000 + '0';
                    case 5:
                        tempString[4] = (loopIndex % 10000)/1000 + '0';
                    case 4:
                        tempString[5] = (loopIndex % 1000)/100 + '0';
                    case 3:
                        tempString[6] = (loopIndex % (uint8_t)100)/10 + '0';
                    case 2:
                        tempString[7] = loopIndex % (uint8_t)10 + '0';
                        break;
                }

                // See if the file is found
                if(FILEIO_FindShortFileName (&directory, filePtr, tempString, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH) == FILEIO_ERROR_DONE)
                {
                    memcpy (filePtr->name, tempString, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);
                    return true;
                    break;
                }
            }
        }
    }
    else
    {
        return true;
    }

    return false;
}
#endif

void FILEIO_ShortFileNameGet (FILEIO_OBJECT * filePtr, char * buffer)
{
    FILEIO_ShortFileNameConvert (buffer, filePtr->name);
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryPopulate(FILEIO_OBJECT * filePtr, uint16_t * entryHandle, uint8_t attributes, uint32_t cluster)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_DIRECTORY directory;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    FILEIO_TIMESTAMP timeStamp;

    directory.cluster = filePtr->baseClusterDir;
    directory.drive = filePtr->disk;

    currentCluster = filePtr->baseClusterDir;

    entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, *entryHandle);

    if (entry == NULL)
    {
        ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_BAD_CACHE_READ;
        return FILEIO_ERROR_BAD_CACHE_READ;
    }

    strncpy (entry->name, filePtr->name, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);

    entry->attributes = attributes;
    entry->reserved0 = 0x00;
    entry->firstClusterLow = (cluster & 0x0000FFFF);
    entry->firstClusterHigh = (cluster & 0x0FFF0000) >> 16;     // FAT32 only uses 28 bits of the upper word.  Mask off the other four bits
    entry->fileSize = 0x00000000;

    if (timestampGet != NULL)
    {
        (*timestampGet)(&timeStamp);
    }

    entry->createTimeMs = timeStamp.timeMs;
    entry->createTime = timeStamp.time.value;
    entry->createDate = timeStamp.date.value;
    entry->writeTime = timeStamp.time.value;
    entry->writeDate = timeStamp.date.value;

    // Populate the file object
    filePtr->firstCluster = cluster;
    filePtr->currentCluster = cluster;
    filePtr->currentSector = 0;
    filePtr->currentOffset = 0;
    filePtr->absoluteOffset = 0;
    filePtr->size = entry->fileSize;
    filePtr->attributes = entry->attributes;
    if ((filePtr->attributes & FILEIO_ATTRIBUTE_DIRECTORY) == FILEIO_ATTRIBUTE_DIRECTORY)
    {
        filePtr->timeMs = entry->createTimeMs;
        filePtr->time = entry->createTime;
        filePtr->date = entry->createDate;
    }
    else
    {
        filePtr->time = entry->writeTime;
        filePtr->date = entry->writeDate;
    }
    filePtr->entry = *entryHandle;
    filePtr->baseClusterDir = directory.cluster;
    filePtr->currentClusterDir = directory.cluster;

    ((FILEIO_DRIVE *)filePtr->disk)->bufferStatusPtr->flags.dataBufferNeedsWrite = true;

    return error;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryFindEmpty (FILEIO_OBJECT * filePtr, uint16_t * entryOffset)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_DIRECTORY directory;
    uint32_t currentCluster = filePtr->baseClusterDir;
    uint32_t tempCurrentCluster;
    uint16_t currentClusterOffset = 0;
    uint8_t fileEntryCount;
    uint8_t foundEntryCount;
    uint16_t tempHandle, tempHandle2;
    enum {NOT_FOUND, FOUND, ERROR} status = NOT_FOUND;

    directory.drive = filePtr->disk;
    directory.cluster = filePtr->baseClusterDir;

    if (filePtr->lfnPtr)
    {
        fileEntryCount = (filePtr->lfnLen / FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY) + 1;
        if ((filePtr->lfnLen % FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY) ||
                (filePtr->lfnLen < FILEIO_FILE_NAME_UTF16_CHARS_IN_LFN_ENTRY))
        {
            fileEntryCount++;
        }
    }
    else
    {
        fileEntryCount = 1;
    }

    tempHandle2 = *entryOffset;

    while (status == NOT_FOUND)
    {
        foundEntryCount = 0;
        tempHandle = tempHandle2;

        // Find [fileEntryCount] empty entries
        do
        {
            tempCurrentCluster = currentCluster;
            entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, tempHandle2);

            tempHandle2++;
        } while ((entry != NULL) && (((uint8_t)(entry->name[0]) == FILEIO_DIRECTORY_ENTRY_DELETED) || (entry->name[0] == FILEIO_DIRECTORY_ENTRY_EMPTY)) && (++foundEntryCount < fileEntryCount));
        
        if (entry == NULL)
        {
            if ((currentCluster == ((FILEIO_DRIVE *)filePtr->disk)->firstRootCluster) && (((FILEIO_DRIVE *)filePtr->disk)->type != FILEIO_FILE_SYSTEM_TYPE_FAT32))
            {
                    status = ERROR;
            }
            else
            {
                currentCluster = tempCurrentCluster;
                if (FILEIO_ClusterAllocate (filePtr->disk, &currentCluster, true) == FILEIO_ERROR_DRIVE_FULL)
                {
                    status = ERROR;
                }
                else
                {
                    status = FOUND;
                }
            }
        }
        else if (foundEntryCount == fileEntryCount)
        {
            status = FOUND;
        }
    }

    *entryOffset = tempHandle;

    if (status == FOUND)
    {
        return FILEIO_ERROR_NONE;
    }
    else
    {
        return FILEIO_ERROR_DONE;
    }
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_ClusterAllocate (FILEIO_DRIVE * drive, uint32_t * cluster, bool eraseCluster)
{
    uint32_t newCluster;

    newCluster = FILEIO_FindEmptyCluster (drive, *cluster);
    if (newCluster == 0)
    {
        return FILEIO_ERROR_DRIVE_FULL;
    }

    if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT12)
    {
        FILEIO_FATWrite (drive, newCluster, FILEIO_CLUSTER_VALUE_FAT12_EOF, false);
    }
    else if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
    {
        FILEIO_FATWrite (drive, newCluster, FILEIO_CLUSTER_VALUE_FAT16_EOF, false);
    }
    else if (drive->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)
    {
        FILEIO_FATWrite (drive, newCluster, FILEIO_CLUSTER_VALUE_FAT32_EOF, false);
    }

    FILEIO_FATWrite (drive, *cluster, newCluster, false);

    *cluster = newCluster;

    if (eraseCluster)
    {
        return FILEIO_EraseCluster (drive, newCluster);
    }
    else
    {
        return FILEIO_ERROR_NONE;
    }
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
uint32_t FILEIO_FindEmptyCluster (FILEIO_DRIVE * drive, uint32_t baseCluster)
{
    uint32_t cluster = 0x0;
    uint32_t currentCluster, endClusterLimit, clusterFailValue;

    /* Settings based on FAT type */
    switch (drive->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            endClusterLimit = FILEIO_CLUSTER_VALUE_FAT32_END;
            clusterFailValue = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
            endClusterLimit = FILEIO_CLUSTER_VALUE_FAT12_END;
            clusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            endClusterLimit = FILEIO_CLUSTER_VALUE_FAT16_END;
            clusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            break;
    }

    // just in case
    if(baseCluster < 2)
        baseCluster = 2;

    currentCluster = baseCluster;

    // sequentially scan through the FAT looking for an empty cluster
    while(currentCluster)
    {
        // look at its value
        if ((cluster = FILEIO_FATRead(drive, currentCluster)) == clusterFailValue)
        {
            currentCluster = 0;
            break;
        }

        // check if empty cluster found
        if (cluster == FILEIO_CLUSTER_VALUE_EMPTY)
            break;

        currentCluster++;    // check next cluster in FAT

        // check if reached last cluster in FAT, re-start from top
        if ((cluster == endClusterLimit) || (currentCluster >= (drive->partitionClusterCount + 2)))
        {
            currentCluster = 2;
        }

        // check if full circle done, disk full
        if ( currentCluster == baseCluster)
        {
            currentCluster = 0;
            break;
        }
    }  // scanning for an empty cluster

    return(currentCluster);
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_EraseCluster (FILEIO_DRIVE * drive, uint32_t cluster)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    uint32_t sector = FILEIO_ClusterToSector (drive, cluster);
    uint8_t i;

    if (!FILEIO_FlushBuffer (drive, FILEIO_BUFFER_DATA))
    {
        drive->error = FILEIO_ERROR_WRITE;
        return FILEIO_ERROR_WRITE;
    }

    memset (drive->dataBuffer, 0x00, drive->sectorSize);

    for (i = 0; (i < drive->sectorsPerCluster) && (error == FILEIO_ERROR_NONE); i++)
    {
        if (!(*drive->driveConfig->funcSectorWrite)(drive->mediaParameters, sector++, drive->dataBuffer, false))
        {
            error = FILEIO_ERROR_WRITE;
        }
    }

    // As an optimization, set the cached sector to the first sector of the cluster.  They're all zero anyway, now.
    drive->bufferStatusPtr->dataBufferCachedSector = sector - drive->sectorsPerCluster;
    drive->bufferStatusPtr->flags.dataBufferNeedsWrite = false;

    drive->error = error;
    return error;
}
#endif

FILEIO_DIRECTORY_ENTRY * FILEIO_DirectoryEntryCache (FILEIO_DIRECTORY * directory, FILEIO_ERROR_TYPE * error, uint32_t * currentCluster, uint16_t * currentClusterOffset, uint16_t entryOffset)
{
    FILEIO_DRIVE * disk = directory->drive;
    FILEIO_DIRECTORY_ENTRY * entry;
    uint8_t directoryEntriesPerSector = disk->sectorSize / FILEIO_DIRECTORY_ENTRY_SIZE;
    uint16_t totalSectorOffset = entryOffset / directoryEntriesPerSector;
    uint32_t sector;

    if ((disk->type != FILEIO_FILE_SYSTEM_TYPE_FAT32) && (*currentCluster == FILEIO_FIXED_ROOT_DIRECTORY_CLUSTER_NUMBER))
    {
        if (entryOffset >= disk->rootDirectoryEntryCount)
        {
            *error = FILEIO_ERROR_DONE;
            return NULL;
        }
        // We're looking for the file in the FAT12/16 root directory
        sector = FILEIO_ClusterToSector (directory->drive, directory->cluster);
        sector += totalSectorOffset;
    }
    else
    {
        // Find the cluster/sector with the specified directory entry offset
        uint16_t totalClusterOffset = totalSectorOffset / disk->sectorsPerCluster;
        if (*currentClusterOffset > totalClusterOffset)
        {
            *currentClusterOffset = 0;
            *currentCluster = directory->cluster;
        }

        totalSectorOffset -= (disk->sectorsPerCluster * (*currentClusterOffset));

        while (*currentClusterOffset < totalClusterOffset)
        {
            *currentCluster = FILEIO_FATRead (disk, *currentCluster);
            // Switch based on FAT type
            switch (disk->type)
            {
                case FILEIO_FILE_SYSTEM_TYPE_FAT32:
                    if (*currentCluster == FILEIO_CLUSTER_VALUE_FAT32_EOF)
                    {
                        *error = FILEIO_ERROR_DONE;
                        return NULL;
                    }
                    break;
                case FILEIO_FILE_SYSTEM_TYPE_FAT12:
                case FILEIO_FILE_SYSTEM_TYPE_FAT16:
                default:
                    if (*currentCluster == FILEIO_CLUSTER_VALUE_FAT16_EOF)
                    {
                        *error = FILEIO_ERROR_DONE;
                        return NULL;
                    }
                    break;
            }
            *currentClusterOffset = *currentClusterOffset + 1;
            totalSectorOffset -= disk->sectorsPerCluster;
        }

        // currentCluster points to the cluster we need.  Convert to sectors and add the sector offset.
        sector = FILEIO_ClusterToSector (disk, *currentCluster);
        sector += totalSectorOffset;
    }

    if (disk->bufferStatusPtr->dataBufferCachedSector != sector)
    {
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
        if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_DATA))
        {
            *error = FILEIO_ERROR_WRITE;
            return NULL;
        }
#endif
        if ((*disk->driveConfig->funcSectorRead) (disk->mediaParameters, sector, disk->dataBuffer) != true)
        {
            *error = FILEIO_ERROR_BAD_SECTOR_READ;
            return NULL;
        }
        else
        {
            disk->bufferStatusPtr->dataBufferCachedSector = sector;
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
            disk->bufferStatusPtr->driveOwner = disk;
#endif
        }
    }

    entry = (FILEIO_DIRECTORY_ENTRY *)((FILEIO_DIRECTORY_ENTRY *)disk->dataBuffer + (entryOffset % directoryEntriesPerSector));

    *error = FILEIO_ERROR_NONE;
    
    return entry;
}

FILEIO_ERROR_TYPE FILEIO_ForceRecache (FILEIO_DRIVE * disk)
{
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
    if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_DATA))
    {
        return FILEIO_ERROR_WRITE;
    }
#endif
    if ((*disk->driveConfig->funcSectorRead) (disk->mediaParameters, disk->bufferStatusPtr->dataBufferCachedSector, disk->dataBuffer) != true)
    {
        return FILEIO_ERROR_BAD_SECTOR_READ;
    }

    return FILEIO_ERROR_NONE;
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
bool FILEIO_FlushBuffer (FILEIO_DRIVE * disk, FILEIO_BUFFER_ID bufferId)
{
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    disk = disk->bufferStatusPtr->driveOwner;

    if (disk == NULL)
    {
        return true;
    }
#endif
    switch (bufferId)
    {
        case FILEIO_BUFFER_DATA:
            if (disk->bufferStatusPtr->flags.dataBufferNeedsWrite)
            {
                if (!(*disk->driveConfig->funcSectorWrite)(disk->mediaParameters, disk->bufferStatusPtr->dataBufferCachedSector, disk->dataBuffer, false) )
                {
                    return false;
                }
                disk->bufferStatusPtr->flags.dataBufferNeedsWrite = false;
            }
            break;
        case FILEIO_BUFFER_FAT:
            if (disk->bufferStatusPtr->flags.fatBufferNeedsWrite)
            {
                uint32_t sector = disk->bufferStatusPtr->fatBufferCachedSector;
                uint8_t i;
                for (i = 0; i < disk->fatCopyCount; i++, sector += disk->fatSectorCount)
                {
                    if (! (*disk->driveConfig->funcSectorWrite)(disk->mediaParameters, sector, disk->fatBuffer, false) )
                    {
                        return false;
                    }
                }
                disk->bufferStatusPtr->flags.fatBufferNeedsWrite = false;
            }
            break;
    }
    return true;
}
#endif

bool FILEIO_ShortFileNameCompare (uint8_t * fileName1, uint8_t * fileName2, uint8_t mode)
{
    if ((mode & FILEIO_SEARCH_PARTIAL_STRING_SEARCH) == FILEIO_SEARCH_PARTIAL_STRING_SEARCH)
    {
        uint8_t character;
        uint8_t i;

        // Compare short filename
        for (i = 0; i < 8; i++)
        {
            character = fileName1[i];
            if (character == '*')
                break;
            if (character != '?')
            {
                if (character != fileName2[i])
                {
                    return false;
                }
            }
        }

        // Compare extension
        for (i = 8; i < FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX; i++)
        {
            character = fileName1[i];
            if (character == '*')
                break;
            if (character != '?')
            {
                if (character != fileName2[i])
                {
                    return false;
                }
            }
        }
    }
    else
    {
        // If we need an exact match, just use memcmp
        return (memcmp(fileName1, fileName2, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX) == 0) ? true : false;
    }

    return true;
}

uint32_t FILEIO_ClusterToSector(FILEIO_DRIVE * disk, uint32_t cluster)
{
    uint32_t sector;

    // Rt: Settings based on FAT type
    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            // In FAT32, there is no separate ROOT region. It is as well stored in DATA region
            sector = (((uint32_t)cluster-2) * disk->sectorsPerCluster) + disk->firstDataSector;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            // The root dir takes up cluster 0 and 1
            if((cluster == 0) || (cluster == 1))
                sector = disk->firstRootSector + cluster;
            else
                sector = (((uint32_t)cluster-2) * disk->sectorsPerCluster) + disk->firstDataSector;
            break;
    }

    return(sector);

}

uint32_t FILEIO_FATRead (FILEIO_DRIVE * disk, uint32_t currentCluster)
{
    uint8_t q;
    uint32_t p, l;  // "l" is the sector Address
    uint32_t c = 0, d, ClusterFailValue,LastClusterLimit;   // ClusterEntries

    /* Settings based on FAT type */
    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            p = (uint32_t)currentCluster * 4;
            q = 0; // "q" not used for FAT32, only initialized to remove a warning
            ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
            LastClusterLimit = FILEIO_CLUSTER_VALUE_FAT32_EOF;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
            p = (uint32_t) currentCluster *3;  // Mulby1.5 to find cluster pos in FAT
            q = p&1;
            p >>= 1;
            ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            LastClusterLimit = FILEIO_CLUSTER_VALUE_FAT12_EOF;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            p = (uint32_t)currentCluster *2;     // Mulby 2 to find cluster pos in FAT
            q = 0; // "q" not used for FAT16, only initialized to remove a warning
            ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            LastClusterLimit = FILEIO_CLUSTER_VALUE_FAT16_EOF;
            break;
    }

    l = disk->firstFatSector + (p / disk->sectorSize);     //
    p &= disk->sectorSize - 1;                 // Restrict 'p' within the FATbuffer size

    // Check if the appropriate FAT sector is already loaded
    if (disk->bufferStatusPtr->fatBufferCachedSector == l)
    {
        if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)
        {
            c = ReadRam32bit (disk->fatBuffer, p);
        }
        else
        {
            if(disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
            {
                c = ReadRam16bit (disk->fatBuffer, p);
            }
            else if(disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT12)
            {
                c = *(disk->fatBuffer + p);
                if (q)
                {
                    c >>= 4;
                }
                // Check if the MSB is across the sector boundry
                p = (p +1) & (disk->sectorSize-1);
                if (p == 0)
                {
                    // Start by writing the sector we just worked on to the card
                    // if we need to
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
                    if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_FAT))
                    {
                        return ClusterFailValue;
                    }
#endif
                    if (!(*disk->driveConfig->funcSectorRead) (disk->mediaParameters, l+1, disk->fatBuffer))
                    {
                        disk->bufferStatusPtr->fatBufferCachedSector = 0xFFFFFFFF;
                        return ClusterFailValue;
                    }
                    else
                    {
                        disk->bufferStatusPtr->fatBufferCachedSector = l + 1;
                    }
                }
                d = *(disk->fatBuffer + p);
                if (q)
                {
                    c += (d <<4);
                }
                else
                {
                    c += ((d & 0x0F)<<8);
                }
            }
        }
    }
    else
    {
        // If there's a currently open FAT sector,
        // write it back before reading into the buffer
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
        if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_FAT))
        {
            return ClusterFailValue;
        }
#endif
        if (!(*disk->driveConfig->funcSectorRead) (disk->mediaParameters, l, disk->fatBuffer))
        {
            disk->bufferStatusPtr->fatBufferCachedSector = 0xFFFFFFFF;  // Note: It is Sector not Cluster.
            return ClusterFailValue;
        }
        else
        {
            disk->bufferStatusPtr->fatBufferCachedSector = l;

            if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)
            {
                c = ReadRam32bit (disk->fatBuffer, p);
            }
            else
            {
                if(disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
                {
                    c = ReadRam16bit (disk->fatBuffer, p);
                }
                else if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT12)
                {
                    c = *(disk->fatBuffer + p);
                    if (q)
                    {
                        c >>= 4;
                    }
                    p = (p +1) & (disk->sectorSize-1);
                    d = *(disk->fatBuffer + p);
                    if (q)
                    {
                        c += (d <<4);
                    }
                    else
                    {
                        c += ((d & 0x0F)<<8);
                    }
                }
            }
        }
    }

    // Normalize it so 0xFFFF is an error
    if (c >= LastClusterLimit)
    {
        c = LastClusterLimit;
    }

   return c;
}   // ReadFAT

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_EraseFile (FILEIO_OBJECT * filePtr, uint16_t * entryHandle, bool eraseData)
{
    FILEIO_ERROR_TYPE error;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_DRIVE * disk = filePtr->disk;
    FILEIO_DIRECTORY directory;
    uint32_t currentCluster;
    uint16_t currentClusterOffset;
    uint16_t tempEntryHandle = *entryHandle;
    uint8_t sequenceNumber;

    error = FILEIO_ERROR_ERASE_FAIL;

    directory.drive = filePtr->disk;
    directory.cluster = filePtr->baseClusterDir;
    currentCluster = filePtr->baseClusterDir;
    if (currentCluster == 0)
    {
        currentCluster = disk->firstRootCluster;
    }
    currentClusterOffset = 0;

    // Cache the short file entry
    entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, tempEntryHandle);

    do
    {
        if (entry == NULL)
        {
            disk->error = error;
            return error;
        }

        sequenceNumber = *((uint8_t *)entry);

        if (((uint8_t)entry->name[0] == FILEIO_DIRECTORY_ENTRY_DELETED) || (entry->name[0] == FILEIO_DIRECTORY_ENTRY_EMPTY))
        {
            disk->error = FILEIO_ERROR_FILE_NOT_FOUND;
            return FILEIO_ERROR_FILE_NOT_FOUND;
        }
        else
        {
            entry->name[0] = FILEIO_DIRECTORY_ENTRY_DELETED;
            // Mark the cached sector as needing a write.
            disk->bufferStatusPtr->flags.dataBufferNeedsWrite = true;
        }

        if (((entry->attributes == FILEIO_ATTRIBUTE_LONG_NAME) && ((sequenceNumber & 0x40) == 0x40)) || (tempEntryHandle == 0))
        {
            break;
        }

        tempEntryHandle--;

        // Cache the previous entry
        entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, tempEntryHandle);
    } while (entry->attributes == FILEIO_ATTRIBUTE_LONG_NAME);

    if (error == FILEIO_ERROR_NONE)
    {
        // Check to make sure someone isn't trying to erase the root directory.  This should never happen.
        if (filePtr->firstCluster != disk->firstRootCluster)
        {
            if (eraseData)
            {
                error = FILEIO_EraseClusterChain (filePtr->firstCluster, disk) ? FILEIO_ERROR_NONE : FILEIO_ERROR_ERASE_FAIL;
            }
        }
    }

    if (!FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_DATA))
    {
        ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_WRITE;
        return FILEIO_RESULT_FAILURE;
    }

    if (!FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_FAT))
    {
        ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_WRITE;
        return FILEIO_RESULT_FAILURE;
    }

    disk->error = error;
    return error;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
FILEIO_ERROR_TYPE FILEIO_EraseClusterChain (uint32_t cluster, FILEIO_DRIVE * disk)
{
    uint32_t nextCluster, clusterFailed, clusterFinal;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;

    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            clusterFailed = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
            clusterFinal = FILEIO_CLUSTER_VALUE_FAT32_EOF;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
            clusterFailed = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            clusterFinal = FILEIO_CLUSTER_VALUE_FAT12_EOF;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            clusterFailed = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            clusterFinal = FILEIO_CLUSTER_VALUE_FAT16_EOF;
            break;
    }

    if ((cluster == 0) || (cluster == 1))
    {
        error = FILEIO_ERROR_INVALID_ARGUMENT;
    }
    else
    {
        while (error == FILEIO_ERROR_NONE)
        {
            if ((nextCluster = FILEIO_FATRead (disk, cluster)) == clusterFailed)
            {
                error = FILEIO_ERROR_DONE;
            }
            else
            {
                if ((nextCluster == 0) || (nextCluster == 1))
                {
                    error = FILEIO_ERROR_INVALID_ARGUMENT;
                }
                else
                {
                    if (nextCluster >= clusterFinal)
                    {
                        error = FILEIO_ERROR_DONE;
                    }

                    if (FILEIO_FATWrite (disk, cluster, FILEIO_CLUSTER_VALUE_EMPTY, false) == clusterFailed)
                    {
                        error = FILEIO_ERROR_WRITE;
                    }

                    cluster = nextCluster;
                }
            }
        }
    }

    FILEIO_FATWrite (disk, 0, 0, true);

    return error;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
uint32_t FILEIO_FATWrite (FILEIO_DRIVE *disk, uint32_t currentCluster, uint32_t value, uint8_t forceWrite)
{
    uint8_t q, c;
    uint32_t p, l, clusterFailValue;
    FILEIO_BUFFER_STATUS * statusPtr = disk->bufferStatusPtr;

    if ((disk->type != FILEIO_FILE_SYSTEM_TYPE_FAT32) && (disk->type != FILEIO_FILE_SYSTEM_TYPE_FAT16) && (disk->type != FILEIO_FILE_SYSTEM_TYPE_FAT12))
    {
        return FILEIO_CLUSTER_VALUE_FAT32_FAIL;
    }

    /* Settings based on FAT type */
    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            clusterFailValue = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            clusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            break;
    }

    // The only purpose for calling this function with forceWrite
    // is to write the current FAT sector to the card
    if (forceWrite)
    {
        FILEIO_FlushBuffer (disk, FILEIO_BUFFER_FAT);
        return 0;
    }

    /* Settings based on FAT type */
    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            p = (uint32_t)currentCluster *4;   // "p" is the position in "gFATBuffer" for corresponding cluster.
            q = 0;      // "q" not used for FAT32, only initialized to remove a warning
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
            p = (uint32_t) currentCluster * 3; // "p" is the position in "gFATBuffer" for corresponding cluster.
            q = p & 1;   // Odd or even?
            p >>= 1;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            p = (uint32_t) currentCluster *2;   // "p" is the position in "gFATBuffer" for corresponding cluster.
            q = 0;      // "q" not used for FAT16, only initialized to remove a warning
            break;
    }

    l = disk->firstFatSector + (p / disk->sectorSize);     //
    p &= disk->sectorSize - 1;                 // Restrict 'p' within the FATbuffer size

    if (disk->bufferStatusPtr->fatBufferCachedSector != l)
    {
        // If we are loading a new sector then write
        // the current one to the card if we need to
        if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_FAT))
        {
            return clusterFailValue;
        }

        // Load the new sector
        if (!(*disk->driveConfig->funcSectorRead) (disk->mediaParameters, l, disk->fatBuffer))
        {
            statusPtr->fatBufferCachedSector = 0xFFFFFFFF;
            return clusterFailValue;
        }
        else
        {
            statusPtr->fatBufferCachedSector = l;
        }
    }

    if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)  // Refer page 16 of FAT requirement.
    {
        *(disk->fatBuffer + p) = ((value & 0x000000ff));         // lsb,1st uint8_t of cluster value
        *(disk->fatBuffer + p+1) = ((value & 0x0000ff00) >> 8);
        *(disk->fatBuffer + p+2) = ((value & 0x00ff0000) >> 16);
        *(disk->fatBuffer + p+3) = ((value & 0x0f000000) >> 24);   // the MSB nibble is supposed to be "0" in FAT32. So mask it.
    }
    else
    {
        if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
        {
            *(disk->fatBuffer+ p) = value;            //lsB
            *(disk->fatBuffer + p+1) = ((value&0x0000ff00) >> 8);    // msB
        }
        else if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT12)
        {
            // Get the current uint8_t from the FAT
            c = *(disk->fatBuffer + p);
            if (q)
            {
                c = ((value & 0x0F) << 4) | ( c & 0x0F);
            }
            else
            {
                c = (value & 0xFF);
            }
            // Write in those bits
            *(disk->fatBuffer + p) = c;

            // FAT12 entries can cross sector boundaries
            // Check if we need to load a new sector
            p = (p +1) & (disk->sectorSize-1);
            if (p == 0)
            {
                // call this function to update the FAT on the card
                if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_FAT))
                {
                    return clusterFailValue;
                }

                // Load the next sector
                if (!(*disk->driveConfig->funcSectorRead) (disk->mediaParameters, l +1, disk->fatBuffer))
                {
                    statusPtr->fatBufferCachedSector = 0xFFFFFFFF;
                    return clusterFailValue;
                }
                else
                {
                    statusPtr->fatBufferCachedSector = l + 1;
                }
            }

            // Get the second uint8_t of the table entry
            c = *(disk->fatBuffer + p);
            if (q)
            {
                c = (value >> 4);
            }
            else
            {
                c = ((value >> 8) & 0x0F) | (c & 0xF0);
            }
            *(disk->fatBuffer + p) = c;
        }
    }
    statusPtr->flags.fatBufferNeedsWrite = true;

    return 0;
}
#endif

FILEIO_ERROR_TYPE FILEIO_NextClusterGet (FILEIO_OBJECT * fo, uint32_t count)
{
    uint32_t nextCluster, currentCluster, clusterFailValue, lastClustervalue;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DRIVE * disk;

    disk = fo->disk;

    /* Settings based on FAT type */
    switch (disk->type)
    {
        case FILEIO_FILE_SYSTEM_TYPE_FAT32:
            lastClustervalue = FILEIO_CLUSTER_VALUE_FAT32_EOF;
            clusterFailValue  = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT12:
            lastClustervalue = FILEIO_CLUSTER_VALUE_FAT12_EOF;
            clusterFailValue  = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            break;
        case FILEIO_FILE_SYSTEM_TYPE_FAT16:
        default:
            lastClustervalue = FILEIO_CLUSTER_VALUE_FAT16_EOF;
            clusterFailValue  = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
            break;
    }

    // loop n times
    do
    {
        // get the next cluster link from FAT
        currentCluster = fo->currentCluster;
        if ((nextCluster = FILEIO_FATRead( disk, currentCluster)) == clusterFailValue)
        {
            error = FILEIO_ERROR_BAD_SECTOR_READ;
        }
        else
        {
            // check if cluster value is valid
            if (nextCluster >= (disk->partitionClusterCount + 2))
            {
                error = FILEIO_ERROR_INVALID_CLUSTER;
            }

            // compare against max value of a cluster in FAT
            // return if eof
            if (nextCluster >= lastClustervalue)    // check against eof
            {
                error = FILEIO_ERROR_EOF;
            }
        }

        // update the file object structure
        fo->currentCluster = nextCluster;

    } while ((--count > 0) && (error == FILEIO_ERROR_NONE));// loop end

    return(error);
} // get next cluster


int FILEIO_Close(FILEIO_OBJECT * filePtr)
{
    int result = FILEIO_RESULT_SUCCESS;

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
    result = FILEIO_Flush (filePtr);
#endif

    filePtr->flags.readEnabled = false;
    filePtr->flags.writeEnabled = false;

    return result;
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_Flush (FILEIO_OBJECT * filePtr)
{
    int result = FILEIO_RESULT_SUCCESS;

    FILEIO_TIMESTAMP timeStamp;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_DIRECTORY directory;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    FILEIO_ERROR_TYPE error;

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (filePtr->disk) != FILEIO_RESULT_SUCCESS)
    {
        return 0;
    }
#endif

    if(filePtr->flags.writeEnabled)
    {
        // Write the current data sector to the disk
        if (!FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_DATA))
        {
            ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_WRITE;
            return FILEIO_RESULT_FAILURE;
        }

        // Write the current FAT sector to the disk
        if (!FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_FAT))
        {
            ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_WRITE;
            return FILEIO_RESULT_FAILURE;
        }

        // Read the FAT entry from the physical media.  This is required because
        //   some physical media cache the entries in RAM and only write them
        //   after a time expires for until the sector is accessed again.
        FILEIO_FATRead (filePtr->disk, filePtr->currentCluster);

        directory.drive = filePtr->disk;
        directory.cluster = filePtr->baseClusterDir;
        currentCluster = filePtr->baseClusterDir;
        // Get the file entry
        entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, filePtr->entry);

        if (entry == NULL)
        {
            ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_BAD_CACHE_READ;
            return FILEIO_RESULT_FAILURE;
        }

        if (timestampGet != NULL)
        {
            (*timestampGet)(&timeStamp);
        }

        // update the time
        entry->writeTime = timeStamp.time.value;
        entry->writeDate = timeStamp.date.value;

        entry->fileSize = filePtr->size;

        entry->attributes = filePtr->attributes;

        ((FILEIO_DRIVE *)filePtr->disk)->bufferStatusPtr->flags.dataBufferNeedsWrite = true;

        // just write the last entry in
        if(FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_DATA))
        {
            // Read the folder entry from the physical media.  This is required because
            //   some physical media cache the entries in RAM and only write them
            //   after a time expires for until the sector is accessed again.
            ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ForceRecache (filePtr->disk);
            if (((FILEIO_DRIVE *)filePtr->disk)->error == FILEIO_ERROR_NONE)
            {
                result = FILEIO_RESULT_SUCCESS;
            }
            else
            {
                result = FILEIO_RESULT_FAILURE;
            }
        }
        else
        {
            ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_WRITE;
            result = FILEIO_RESULT_FAILURE;
        }
    }

    return result;
}
#endif

long FILEIO_Tell (FILEIO_OBJECT * filePtr)
{
    ((FILEIO_DRIVE *)filePtr->disk)->error = FILEIO_ERROR_NONE;
    return (filePtr->absoluteOffset);
}

int FILEIO_Seek(FILEIO_OBJECT * filePtr, int32_t offset, int whence)
{
    uint32_t    numsector, temp;   // lba of first sector of first cluster
    FILEIO_DRIVE*   disk;            // pointer to disk structure
    uint8_t   test;
    long offset2 = offset;

    disk = filePtr->disk;

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (disk) != FILEIO_RESULT_SUCCESS)
    {
        return FILEIO_RESULT_FAILURE;
    }
#endif

    switch(whence)
    {
        case FILEIO_SEEK_CUR:
            // Apply the offset to the current position
            offset2 += filePtr->absoluteOffset;
            break;
        case FILEIO_SEEK_END:
            // Apply the offset to the end of the file
            offset2 = filePtr->size - offset2;
            break;
        case FILEIO_SEEK_SET:
            // automatically there
        default:
            break;
   }

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
    if (!FILEIO_FlushBuffer (filePtr->disk, FILEIO_BUFFER_DATA))
    {
        disk->error = FILEIO_ERROR_WRITE;
        return FILEIO_RESULT_FAILURE;
    }
#endif

    // start from the beginning
    filePtr->currentCluster = filePtr->firstCluster;

    if (offset2 > filePtr->size)
    {
        disk->error = FILEIO_ERROR_INVALID_ARGUMENT;
        return FILEIO_RESULT_FAILURE;      // past the limits
    }
    else
    {
        // set the new postion
        filePtr->absoluteOffset = offset2;

        // figure out how many sectors
        numsector = offset2 / disk->sectorSize;

        // figure out how many uint8_ts off of the offset
        offset2 = offset2 - (numsector * disk->sectorSize);
        filePtr->currentOffset = offset2;

        // figure out how many clusters
        temp = numsector / disk->sectorsPerCluster;

        // figure out the stranded sectors
        numsector = numsector - (disk->sectorsPerCluster * temp);
        filePtr->currentSector = numsector;

        // if we are in the current cluster stay there
        if (temp > 0)
        {
            test = FILEIO_NextClusterGet(filePtr, temp);
            if (test != FILEIO_ERROR_NONE)
            {
                if (test == FILEIO_ERROR_EOF)
                {
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
                    if (filePtr->flags.writeEnabled)
                    {
                        // load the previous cluster
                        filePtr->currentCluster = filePtr->firstCluster;
                        // Don't perform this operation if there's only one cluster
                        if (temp != 1)
                        {
                            test = FILEIO_NextClusterGet(filePtr, temp - 1);
                        }
                        if (FILEIO_ClusterAllocate (disk, &filePtr->currentCluster, false) != FILEIO_ERROR_NONE)
                        {
                            disk->error = FILEIO_ERROR_COULD_NOT_GET_CLUSTER;
                            return FILEIO_RESULT_FAILURE;
                        }
                        // sec and currentOffset should already be zero
                    }
                    else
#endif
                    {
                        filePtr->currentCluster = filePtr->firstCluster;
                        if (temp != 1)
                        {
                            test = FILEIO_NextClusterGet(filePtr, temp - 1);
                        }
                        else
                        {
                            test = FILEIO_ERROR_NONE;
                        }
                        if (test != FILEIO_ERROR_NONE)
                        {
                            disk->error = FILEIO_ERROR_COULD_NOT_GET_CLUSTER;
                            return FILEIO_RESULT_FAILURE;
                        }
                        filePtr->currentOffset = disk->sectorSize;
                        filePtr->currentSector = disk->sectorsPerCluster - 1;
                    }
                }
                else
                {
                    disk->error = FILEIO_ERROR_COULD_NOT_GET_CLUSTER;
                    return FILEIO_RESULT_FAILURE;   // past the limits
                }
            }
        }

        // Determine the lba of the selected sector and load
        temp = FILEIO_ClusterToSector(disk, filePtr->currentCluster);

        // now the extra sectors
        numsector = filePtr->currentSector;
        temp += numsector;

        if(!(*disk->driveConfig->funcSectorRead)(disk->mediaParameters, temp, disk->dataBuffer) )
        {
            disk->error = FILEIO_ERROR_BAD_CACHE_READ;
            return FILEIO_RESULT_FAILURE;   // Bad read
        }
        disk->bufferStatusPtr->dataBufferCachedSector = temp;
    }

    disk->error = FILEIO_ERROR_NONE;

    return FILEIO_RESULT_SUCCESS;
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
size_t FILEIO_Write (const void * buffer, size_t size, size_t count, FILEIO_OBJECT * filePtr)
{
    FILEIO_ERROR_TYPE error;
    uint8_t * data = (uint8_t *) buffer;
    FILEIO_DRIVE * disk = filePtr->disk;
    uint32_t currentSector;
    size_t dataWritten = 0;
    uint16_t writeCount;
    size_t length = size * count;

    if (!filePtr->flags.writeEnabled)
    {
        disk->error = FILEIO_ERROR_READ_ONLY;
        return 0;
    }

    // Check to see if the user has passed in no data
    if (length == 0)
    {
        disk->error = FILEIO_ERROR_NONE;
        return 0;
    }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (disk) != FILEIO_RESULT_SUCCESS)
    {
        return 0;
    }
#endif

    if ((*disk->driveConfig->funcWriteProtectGet)(disk->mediaParameters))
    {
        disk->error = FILEIO_ERROR_WRITE_PROTECTED;
        return 0;
    }

    while (length != 0)
    {
        if (filePtr->currentOffset == disk->sectorSize)
        {
            filePtr->currentOffset = 0;
            filePtr->currentSector++;
            if (filePtr->currentSector == disk->sectorsPerCluster)
            {
                uint32_t tempCluster = filePtr->currentCluster;
                filePtr->currentSector = 0;
                // Load/allocate the next cluster
                if ((error = FILEIO_NextClusterGet (filePtr, 1)) == FILEIO_ERROR_EOF)
                {
                    filePtr->currentCluster = tempCluster;
                    // Allocate a new cluster
                    error = FILEIO_ClusterAllocate (disk, &filePtr->currentCluster, false);
                }

                if (error != FILEIO_ERROR_NONE)
                {
                    disk->error = error;
                    return dataWritten;
                }
            }
        }

        currentSector = FILEIO_ClusterToSector (disk, filePtr->currentCluster);
        currentSector += filePtr->currentSector;

        // Cache the required sector, if necessary
        if (disk->bufferStatusPtr->dataBufferCachedSector != currentSector)
        {
            if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_DATA))
            {
                disk->error = FILEIO_ERROR_WRITE;
                return FILEIO_ERROR_WRITE;
            }

            if ((*disk->driveConfig->funcSectorRead) (disk->mediaParameters, currentSector, disk->dataBuffer) != true)
            {
                disk->error = FILEIO_ERROR_BAD_SECTOR_READ;
                return dataWritten;
            }
            else
            {
                disk->bufferStatusPtr->dataBufferCachedSector = currentSector;
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
                disk->bufferStatusPtr->driveOwner = disk;
#endif
            }
        }

        writeCount = ((disk->sectorSize - filePtr->currentOffset) > length) ? length : (disk->sectorSize - filePtr->currentOffset);
        memcpy (disk->dataBuffer + filePtr->currentOffset, data, writeCount);
        disk->bufferStatusPtr->flags.dataBufferNeedsWrite = true;
        data += writeCount;
        filePtr->currentOffset += writeCount;
        dataWritten += writeCount;
        length -= writeCount;
    }

    filePtr->size += dataWritten;
    filePtr->absoluteOffset += dataWritten;

    return dataWritten;
}
#endif

size_t FILEIO_Read (void * buffer, size_t size, size_t count, FILEIO_OBJECT * filePtr)
{
    FILEIO_ERROR_TYPE error;
    uint8_t * data = (uint8_t *) buffer;
    FILEIO_DRIVE * disk = filePtr->disk;
    uint32_t currentSector;
    size_t dataRead = 0;
    uint16_t readCount;
    size_t length = size * count;

    if (!filePtr->flags.readEnabled)
    {
        disk->error = FILEIO_ERROR_WRITE_ONLY;
        return 0;
    }

    // Check to see if the user has passed in no data
    if (length == 0)
    {
        disk->error = FILEIO_ERROR_NONE;
        return 0;
    }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (disk) != FILEIO_RESULT_SUCCESS)
    {
        return 0;
    }
#endif

    while (length != 0)
    {
        if (filePtr->currentOffset == disk->sectorSize)
        {
            filePtr->currentOffset = 0;
            filePtr->currentSector++;
            if (filePtr->currentSector == disk->sectorsPerCluster)
            {
                filePtr->currentSector = 0;
                // Load/allocate the next cluster
                error = FILEIO_NextClusterGet (filePtr, 1);

                if (error != FILEIO_ERROR_NONE)
                {
                    disk->error = error;
                    return dataRead;
                }
            }
        }

        currentSector = FILEIO_ClusterToSector (disk, filePtr->currentCluster);
        currentSector += filePtr->currentSector;

        // Cache the required sector, if necessary
        if (disk->bufferStatusPtr->dataBufferCachedSector != currentSector)
        {
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
            if (!FILEIO_FlushBuffer (disk, FILEIO_BUFFER_DATA))
            {
                disk->error = FILEIO_ERROR_WRITE;
                return FILEIO_ERROR_WRITE;
            }
#endif

            if ((*disk->driveConfig->funcSectorRead) (disk->mediaParameters, currentSector, disk->dataBuffer) != true)
            {
                disk->error = FILEIO_ERROR_BAD_SECTOR_READ;
                return dataRead;
            }
            else
            {
                disk->bufferStatusPtr->dataBufferCachedSector = currentSector;
#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
                disk->bufferStatusPtr->driveOwner = disk;
#endif
            }
        }

        readCount = ((disk->sectorSize - filePtr->currentOffset) > length) ? length : (disk->sectorSize - filePtr->currentOffset);
        if ((filePtr->size - filePtr->absoluteOffset) < readCount)
        {
            readCount = filePtr->size - filePtr->absoluteOffset;
            length = readCount;
        }
        memcpy (data, disk->dataBuffer + filePtr->currentOffset, readCount);
        data += readCount;
        filePtr->currentOffset += readCount;
        filePtr->absoluteOffset += readCount;
        dataRead += readCount;
        length -= readCount;
    }

    return dataRead;
}

bool FILEIO_Eof (FILEIO_OBJECT * filePtr)
{
    return (filePtr->absoluteOffset == filePtr->size) ? true : false;
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_Remove (const uint16_t * pathName)
{
    FILEIO_OBJECT file;
    FILEIO_OBJECT * filePtr = &file;
    FILEIO_ERROR_TYPE error;
    uint16_t entryHandle;
    FILEIO_DIRECTORY directory;
    uint8_t fileNameType;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    uint16_t * fileName;

    fileName = (uint16_t *)FILEIO_CacheDirectory (&directory, (uint16_t *)pathName, false);

    if (fileName == NULL)
    {
        globalParameters.currentWorkingDirectory.drive->error = FILEIO_ERROR_INVALID_ARGUMENT;
        return FILEIO_RESULT_FAILURE;
    }

    currentCluster = directory.cluster;

    if((*directory.drive->driveConfig->funcWriteProtectGet)(directory.drive->mediaParameters))
    {
        directory.drive->error = FILEIO_ERROR_WRITE_PROTECTED;
        return FILEIO_RESULT_FAILURE;
    }

    fileNameType = FILEIO_FileNameTypeGet(fileName, false);

    if ((fileNameType == FILEIO_NAME_INVALID) || (fileNameType == FILEIO_NAME_DOT))
    {
        directory.drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (fileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (fileName, filePtr);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (&directory, filePtr, (uint8_t *)filePtr->name, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (fileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        filePtr->lfnPtr = (uint16_t *)fileName;
        filePtr->lfnLen = FILEIO_strlen16 ((uint16_t *)fileName);
        error = FILEIO_FindLongFileName (&directory, filePtr, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }

    if (error == FILEIO_ERROR_NONE)
    {
        entryHandle = filePtr->entry;
        if ((filePtr->attributes & FILEIO_ATTRIBUTE_DIRECTORY) == FILEIO_ATTRIBUTE_DIRECTORY)
        {
            error = FILEIO_ERROR_DELETE_DIR;
        }
        else
        {
            error = FILEIO_EraseFile (filePtr, &entryHandle, true);
        }
    }

    // Check to ensure no errors occured
    if (error != FILEIO_ERROR_NONE)
    {
        directory.drive->error = error;
        return FILEIO_RESULT_FAILURE;
    }

    return FILEIO_RESULT_SUCCESS;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_Rename (const uint16_t * oldPathname, const uint16_t * newFilename)
{
    FILEIO_OBJECT file;
    FILEIO_OBJECT * filePtr = &file;
    FILEIO_ERROR_TYPE error;
    FILEIO_DIRECTORY_ENTRY * entry;
    uint16_t entryHandle;
    FILEIO_DIRECTORY directory;
    uint8_t oldFileNameType;
    uint8_t newFileNameType;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    uint16_t * oldFilename;

    oldFilename = FILEIO_CacheDirectory (&directory, (uint16_t *)oldPathname, false);

    if (oldFilename == NULL)
    {
        globalParameters.currentWorkingDirectory.drive->error = FILEIO_ERROR_INVALID_ARGUMENT;
        return FILEIO_RESULT_FAILURE;
    }

    currentCluster = directory.cluster;

    if((*directory.drive->driveConfig->funcWriteProtectGet)(directory.drive->mediaParameters))
    {
        directory.drive->error = FILEIO_ERROR_WRITE_PROTECTED;
        return FILEIO_RESULT_FAILURE;
    }

    // Check to see if the new filename already exists
    newFileNameType = FILEIO_FileNameTypeGet(newFilename, false);

    if ((newFileNameType == FILEIO_NAME_INVALID) || (newFileNameType == FILEIO_NAME_DOT))
    {
        directory.drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (newFileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (newFilename, filePtr);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (&directory, filePtr, (uint8_t *)filePtr->name, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (newFileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        filePtr->lfnPtr = (uint16_t *)newFilename;
        filePtr->lfnLen = FILEIO_strlen16 ((uint16_t *)newFilename);
        error = FILEIO_FindLongFileName (&directory, filePtr, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }

    if (error == FILEIO_ERROR_NONE)
    {
        directory.drive->error = FILEIO_ERROR_FILENAME_EXISTS;
        return FILEIO_RESULT_FAILURE;
    }

    // Try to find the old filename
    oldFileNameType = FILEIO_FileNameTypeGet(oldFilename, false);

    currentClusterOffset = 0;
    currentCluster = directory.cluster;

    if ((oldFileNameType == FILEIO_NAME_INVALID) || (oldFileNameType == FILEIO_NAME_DOT))
    {
        directory.drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (oldFileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (oldFilename, filePtr);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (&directory, filePtr, (uint8_t *)filePtr->name, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (oldFileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        filePtr->lfnPtr = (uint16_t *)oldFilename;
        filePtr->lfnLen = FILEIO_strlen16 ((uint16_t *)oldFilename);
        error = FILEIO_FindLongFileName (&directory, filePtr, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);
    }

    // Check to ensure that the old file was found
    if (error != FILEIO_ERROR_NONE)
    {
        directory.drive->error = FILEIO_ERROR_FILE_NOT_FOUND;
        return FILEIO_RESULT_FAILURE;
    }

    entryHandle = filePtr->entry;

    // The file was found.  Replace the name.
    if ((oldFileNameType == FILEIO_NAME_SHORT) && (newFileNameType == FILEIO_NAME_SHORT))
    {
        entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, entryHandle);
        FILEIO_FormatShortFileName (newFilename, filePtr);
        memcpy (entry->name, filePtr->name, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);
    }
    else
    {
        uint32_t oldCluster = filePtr->firstCluster;
        uint32_t oldSize = filePtr->size;

        // Erase the previous entry
        if (FILEIO_EraseFile (filePtr, &entryHandle, false) != FILEIO_ERROR_NONE)
        {
            directory.drive->error = FILEIO_ERROR_ERASE_FAIL;
            return FILEIO_RESULT_FAILURE;
        }

        if (newFileNameType == FILEIO_NAME_SHORT)
        {
            // Old file name was long, new file name is short
            FILEIO_FormatShortFileName (newFilename, filePtr);
            filePtr->lfnPtr = NULL;
        }
        else
        {
            // Old file name was short or long, new file name is long
            filePtr->lfnPtr = (uint16_t *)newFilename;
            filePtr->lfnLen = FILEIO_strlen16 ((uint16_t *)newFilename);
        }

        error = FILEIO_DirectoryEntryCreate (filePtr, &entryHandle, filePtr->attributes, false);

        entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, entryHandle);

        // Store the existing file's location and size in the new entry
        entry->firstClusterLow = (oldCluster & 0x0000FFFF);
        entry->firstClusterHigh = (oldCluster & 0x0FFF0000) >> 16;
        entry->fileSize = oldSize;
    }

    directory.drive->bufferStatusPtr->flags.dataBufferNeedsWrite = true;

    // Try to flush the buffer (since the user can't manually flush the data to ensure it writes immediately)
    if (!FILEIO_FlushBuffer (directory.drive, FILEIO_BUFFER_DATA))
    {
        directory.drive->error = FILEIO_ERROR_WRITE;
        return FILEIO_RESULT_FAILURE;
    }

    return FILEIO_RESULT_SUCCESS;
}
#endif

FILEIO_ERROR_TYPE FILEIO_ErrorGet (uint16_t driveId)
{
    FILEIO_DRIVE * drive = FILEIO_CharToDrive (driveId);

    if (drive == NULL)
    {
        return FILEIO_ERROR_DRIVE_NOT_FOUND;
    }
    else
    {
        return drive->error;
    }
}

void FILEIO_ErrorClear (uint16_t driveId)
{
    FILEIO_DRIVE * drive = FILEIO_CharToDrive (driveId);

    if (drive != NULL)
    {
        drive->error = FILEIO_ERROR_NONE;
    }
}

int FILEIO_GetChar (FILEIO_OBJECT * handle)
{
    char c;

    if (FILEIO_Read(&c, 1, 1, handle) != 1)
    {
        return FILEIO_RESULT_FAILURE;
    }
    else
    {
        return (int)c;
    }
}

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_PutChar (char c, FILEIO_OBJECT * handle)
{
    if (FILEIO_Write(&c, 1, 1, handle) != 1)
    {
        return FILEIO_RESULT_FAILURE;
    }
    else
    {
        return FILEIO_RESULT_SUCCESS;
    }
}
#endif

#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
int FILEIO_DirectoryChange (const uint16_t * path)
{
    uint16_t * finalPath;
    FILEIO_DIRECTORY directory;
    uint16_t pathLen;

    finalPath = FILEIO_CacheDirectory (&directory, (uint16_t *)path, false);

    if (finalPath == NULL)
    {
        return FILEIO_RESULT_FAILURE;
    }

    pathLen = FILEIO_strlen16 (finalPath);

    if (pathLen != 0)
    {
        // Try to change to the final directory
        if (FILEIO_DirectoryChangeSingle (&directory, finalPath) != FILEIO_RESULT_SUCCESS)
        {
            return FILEIO_RESULT_FAILURE;
        }
    }

    // Directory was changed successfully
    globalParameters.currentWorkingDirectory.drive = directory.drive;
    globalParameters.currentWorkingDirectory.cluster = directory.cluster;

    return FILEIO_RESULT_SUCCESS;
}
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
int FILEIO_DirectoryMake (const uint16_t * path)
{
    uint16_t * finalPath;
    FILEIO_DIRECTORY directory;
    uint16_t pathLen;

    finalPath = FILEIO_CacheDirectory (&directory, (uint16_t *)path, true);

    if (finalPath == NULL)
    {
        return FILEIO_RESULT_FAILURE;
    }

    pathLen = FILEIO_strlen16 (finalPath);

    if (pathLen != 0)
    {
        // Try to change to the final directory
        if (FILEIO_DirectoryChangeSingle (&directory, finalPath) != FILEIO_RESULT_SUCCESS)
        {
            // If we couldn't change to that directory, it probably doesn't exist.  Try to create it.
            if (FILEIO_DirectoryMakeSingle (&directory, finalPath) != FILEIO_RESULT_SUCCESS)
            {
                return FILEIO_RESULT_FAILURE;
            }
            // Now that it exists, try to change to it.
            if (FILEIO_DirectoryChangeSingle (&directory, finalPath) != FILEIO_RESULT_SUCCESS)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }
    }

    // Directory were created successfully
    return FILEIO_RESULT_SUCCESS;
}
#endif
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
int FILEIO_DirectoryRemove (const uint16_t * path)
{
    uint16_t * finalPath;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_ERROR_TYPE error;
    FILEIO_DIRECTORY directory;
    FILEIO_DIRECTORY deletedDirectory;
    uint16_t pathLen;
    uint32_t currentCluster;
    uint16_t currentClusterOffset = 0;
    uint16_t entryOffset = 2;

    finalPath = FILEIO_CacheDirectory (&directory, (uint16_t *)path, false);

    memcpy (&deletedDirectory, &directory, sizeof (FILEIO_DIRECTORY));

    if (finalPath == NULL)
    {
        return FILEIO_RESULT_FAILURE;
    }

    if ((*directory.drive->driveConfig->funcWriteProtectGet)(directory.drive->mediaParameters))
    {
        return FILEIO_RESULT_FAILURE;
    }

    // Change to the final directory (if the user didn't terminate the path with a delimiter)
    pathLen = FILEIO_strlen16 (finalPath);
    if (pathLen != 0)
    {
        if (FILEIO_DirectoryChangeSingle (&deletedDirectory, finalPath) != FILEIO_RESULT_SUCCESS)
        {
            return FILEIO_RESULT_FAILURE;
        }
    }

    // Don't let the user try to remove the root directory
    if (deletedDirectory.cluster == deletedDirectory.drive->firstRootCluster)
    {
        return FILEIO_RESULT_FAILURE;
    }

    // Check to make sure that the final directory is empty so we don't orphan files
    currentCluster = deletedDirectory.cluster;
    do
    {
        entry = FILEIO_DirectoryEntryCache (&deletedDirectory, &error, &currentCluster, &currentClusterOffset, entryOffset);
        if (entry == NULL)
        {
            return FILEIO_RESULT_FAILURE;
        }

        if ((entry->name[0] != FILEIO_DIRECTORY_ENTRY_EMPTY) && ((uint8_t)(entry->name[0]) != FILEIO_DIRECTORY_ENTRY_DELETED))
        {
            return FILEIO_RESULT_FAILURE;
        }
    } while (entry->name[0] != FILEIO_DIRECTORY_ENTRY_EMPTY);

    return FILEIO_DirectoryRemoveSingle (&directory, finalPath);
}
#endif
#endif

#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
int FILEIO_DirectoryRemoveSingle (FILEIO_DIRECTORY * directory, uint16_t * path)
{
    FILEIO_OBJECT file;
    FILEIO_ERROR_TYPE error;
    uint8_t fileNameType;
    uint32_t currentCluster = directory->cluster;
    uint16_t currentClusterOffset = 0;

    fileNameType = FILEIO_FileNameTypeGet(path, false);

    if ((fileNameType == FILEIO_NAME_INVALID) || (fileNameType == FILEIO_NAME_DOT))
    {
		directory->drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (fileNameType == FILEIO_NAME_SHORT)
    {
        currentCluster = directory->cluster;
        currentClusterOffset = 0;
        // Short file name
        FILEIO_FormatShortFileName (path, &file);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (directory, &file, (uint8_t *)file.name, &currentCluster, &currentClusterOffset, 0, FILEIO_ATTRIBUTE_MASK, FILEIO_SEARCH_ENTRY_MATCH);
    }
    else if (fileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        currentCluster = directory->cluster;
        currentClusterOffset = 0;
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnData
        file.lfnPtr = (uint16_t *)path;
        file.lfnLen = FILEIO_strlen16 ((uint16_t *)path);
        error = FILEIO_FindLongFileName (directory, &file, &currentCluster, &currentClusterOffset, 0, 0, FILEIO_SEARCH_ENTRY_MATCH);

    }

    if (error != FILEIO_ERROR_NONE)
    {
        return FILEIO_RESULT_FAILURE;
    }

    if ((file.attributes & FILEIO_ATTRIBUTE_DIRECTORY) != FILEIO_ATTRIBUTE_DIRECTORY)
    {
        directory->drive->error = FILEIO_ERROR_DELETE_FILE;
        return FILEIO_RESULT_FAILURE;
    }

    if (FILEIO_EraseFile (&file, &file.entry, true) == FILEIO_ERROR_NONE)
    {
        return FILEIO_RESULT_SUCCESS;
    }
    else
    {
        return FILEIO_RESULT_FAILURE;
    }
}
#endif
#endif

#if !defined (FILEIO_CONFIG_DIRECTORY_DISABLE)
uint16_t FILEIO_DirectoryGetCurrent (uint16_t * buffer, uint16_t size)
{
    uint16_t * bufferEnd;
    FILEIO_DRIVE * drive = globalParameters.currentWorkingDirectory.drive;
    uint32_t cluster = globalParameters.currentWorkingDirectory.cluster;
    uint32_t currentCluster, currentClusterTemp;
    uint16_t currentClusterOffset;
    uint16_t entryOffset;
    FILEIO_DIRECTORY directory;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_ERROR_TYPE error;
    uint32_t tempCluster;
    int16_t i = 0, index = 0, tempIndex;
    int16_t j;
    bool bufferOverflow = false;
    char aChar;
    uint16_t charCount = 0;
    const uint16_t dotdotPath[3] = {'.', '.', 0};
    uint8_t checksum;
    uint8_t * source;

    memcpy (&directory, &globalParameters.currentWorkingDirectory, sizeof (FILEIO_DIRECTORY));

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (drive) != FILEIO_RESULT_SUCCESS)
    {
        return 0;
    }
#endif

    drive->error = FILEIO_ERROR_NONE;

    // Set up the return value
    if ((buffer == NULL) || (size == 0))
    {
        drive->error = FILEIO_ERROR_INVALID_ARGUMENT;
        return 0;
    }

    bufferEnd = buffer + size - 1;

    // Loop backwards though all subdirectories
    while ((cluster != 0) && (cluster != drive->firstRootCluster))
    {
        // Change to parent directory
        if (FILEIO_DirectoryChangeSingle(&directory, (uint16_t *)dotdotPath) == FILEIO_RESULT_FAILURE)
        {
            drive->error = FILEIO_ERROR_DIR_NOT_FOUND;
            return 0;
        }

        // Try to find the directory entry for our directory
        currentCluster = directory.cluster;
        currentClusterOffset = 0;
        if (currentCluster == 0)
        {
            currentCluster = drive->firstRootCluster;
            entryOffset = 0;
        }
        else
        {
            entryOffset = 2;
        }

        do
        {
            entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, entryOffset++);
            if (((tempCluster = FILEIO_FullClusterNumberGet(entry)) == cluster) && ((uint8_t)entry->name[0] != FILEIO_DIRECTORY_ENTRY_DELETED)&& (entry->attributes != FILEIO_ATTRIBUTE_LONG_NAME))
            {
                break;
            }
        } while ((entry != NULL) && (entry->name[0] != FILEIO_DIRECTORY_ENTRY_EMPTY));

        if (tempCluster != cluster)
        {
            drive->error = FILEIO_ERROR_DIR_NOT_FOUND;
            return 0;
        }

        cluster = directory.cluster;

        checksum = 0;
        source = (uint8_t *)entry->name;

        for (i = 11; i != 0; i--)
        {
            checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *source++;
        }

        currentClusterTemp = currentCluster;
        if (FILEIO_LongFileNameCache(&directory, entryOffset - 1, currentCluster, checksum) == FILEIO_LFN_SUCCESS)
        {
            // The long file name is cached
            // Copy it backwards into the buffer
            j = FILEIO_strlen16 (lfnBuffer);
            charCount += j;
            for (j -= 1; j >= 0; j--)
            {
                *(buffer + index++) = lfnBuffer[j];
                if (index == size)
                {
                    index = 0;
                    bufferOverflow = true;
                }
            }
        }
        else
        {
            // There's no long file name entry associated with this directory
            // Copy and pad the short file name

            // Ensure that the original entry is still cached
            currentCluster = currentClusterTemp;
            entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, entryOffset - 1);

            // We have found our directory in the parent directory.  Copy the name into the buffer (backwards)
            // Copy the extension.
            j = 10;
            while (entry->name[j] == 0x20)
            {
                j--;
            }
            if (j >= 8)
            {
                charCount += (j - 8) + 1;
                while (j >= 8)
                {
                    *(buffer + index++) = entry->name[j--];
                    // This is a circular buffer
                    // Any unaccomadatable values will be overwritten
                    if (index == size)
                    {
                        index = 0;
                        bufferOverflow = true;
                    }
                }

                charCount++;
                *(buffer + index++) = '.';
                if (index == size)
                {
                    index = 0;
                    bufferOverflow = true;
                }
            }

            // Copy the name
            while (entry->name[j] == 0x20)
            {
                j--;
            }

            charCount += j + 1;
            while (j >= 0)
            {
                *(buffer + index++) = entry->name[j--];
                // This is a circular buffer
                // Any unnecessary values will be overwritten
                if (index == size)
                {
                    index = 0;
                    bufferOverflow = true;
                }
            }
        }

        charCount++;
        *(buffer + index++) = FILEIO_CONFIG_DELIMITER;

        if (index == size)
        {
            index = 0;
            bufferOverflow = true;
        }
    }

    charCount += 2;
    // We have reached the root.  Copy the drive ID into the buffer.
    *(buffer + index++) = ':';
    if (index == size)
    {
        index = 0;
        bufferOverflow = true;
    }
    *(buffer + index++) = drive->driveId;
    if (index == size)
    {
        index = 0;
        bufferOverflow = true;
    }


    // Reverse the contents of the buffer.
    // Point the index back at the last char in the string
    if (index == 0)
    {
        index = size;
    }
    else
    {
        index--;
    }

    i = 0;
    // Swap the chars in the buffer so they are in the right places
    if (bufferOverflow)
    {
        tempIndex = index;
        // Swap the overflowed values in the buffer
        while ((index - i) > 0)
        {
             aChar = *(buffer + i);
             *(buffer + i) = * (buffer + index);
             *(buffer + index) = aChar;
             index--;
             i++;
        }

        // Point at the non-overflowed values
        i = tempIndex + 1;
        index = bufferEnd - buffer;

        // Swap the non-overflowed values into the right places
        while ((index - i) > 0)
        {
             aChar = *(buffer + i);
             *(buffer + i) = * (buffer + index);
             *(buffer + index) = aChar;
             index--;
             i++;
        }
        // All the values should be in the right place now
        // Null-terminate the string
        *(bufferEnd) = 0;
    }
    else
    {
        // There was no overflow, just do one set of swaps
        tempIndex = index;
        while ((index - i) > 0)
        {
            aChar = *(buffer + i);
            *(buffer + i) = * (buffer + index);
            *(buffer + index) = aChar;
            index--;
            i++;
        }
        *(buffer + tempIndex + 1) = 0;
    }

    return charCount;
}
#endif

#if !defined (FILEIO_CONFIG_SEARCH_DISABLE)
int FILEIO_Find (const uint16_t * fileName, unsigned int attr, FILEIO_SEARCH_RECORD * record, bool newSearch)
{
    FILEIO_DIRECTORY directory;
    uint8_t fileNameType;
    FILEIO_ERROR_TYPE error;
    FILEIO_OBJECT file;
    uint16_t * fileWithoutDirectory;

    if (newSearch)
    {
        fileWithoutDirectory = FILEIO_CacheDirectory (&directory, (uint16_t *)fileName, false);

        if (fileWithoutDirectory == NULL)
        {
            globalParameters.currentWorkingDirectory.drive->error = FILEIO_ERROR_INVALID_ARGUMENT;
            return FILEIO_RESULT_FAILURE;
        }

        record->pathOffset = fileWithoutDirectory - fileName;
        record->currentClusterOffset = 0;
        record->currentDirCluster = directory.cluster;
        record->baseDirCluster = directory.cluster;
        record->driveId = directory.drive->driveId;
        record->currentEntryOffset = 0;
    }
    else
    {
        directory.drive = FILEIO_CharToDrive (record->driveId);
        directory.cluster = record->baseDirCluster;

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
        if (FILEIO_GetSingleBuffer (directory.drive) != FILEIO_RESULT_SUCCESS)
        {
            directory.drive->error = FILEIO_ERROR_WRITE;
            return FILEIO_RESULT_FAILURE;
        }
#endif

        fileWithoutDirectory = (uint16_t *)fileName + record->pathOffset;
    }

    fileNameType = FILEIO_FileNameTypeGet(fileWithoutDirectory, true);

    if ((fileNameType == FILEIO_NAME_INVALID) || (fileNameType == FILEIO_NAME_DOT))
    {
        directory.drive->error = FILEIO_ERROR_INVALID_FILENAME;
        return FILEIO_RESULT_FAILURE;
    }
    else if (fileNameType == FILEIO_NAME_SHORT)
    {
        // Short file name
        FILEIO_FormatShortFileName (fileWithoutDirectory, &file);
        // Search in 'directory' for an entry matching filePtr->name, starting at entry 0 in directory->cluster and returning the result in filePtr
        error = FILEIO_FindShortFileName (&directory, &file, (uint8_t *)&file.name, &record->currentDirCluster, &record->currentClusterOffset, record->currentEntryOffset, attr, FILEIO_SEARCH_PARTIAL_STRING_SEARCH | FILEIO_SEARCH_ENTRY_ATTRIBUTES);
    }
    else if (fileNameType == FILEIO_NAME_LONG)
    {
        // Long file name
        // Search in 'directory' for an entry matching fileName, starting at entry 0 in directory->cluster and returning the short file name in filePtr.
        // The long file name will be cached in lfnBuffer
        file.lfnPtr = (uint16_t *)fileWithoutDirectory;
        file.lfnLen = FILEIO_strlen16 ((uint16_t *)fileWithoutDirectory);
        error = FILEIO_FindLongFileName (&directory, &file, &record->currentDirCluster, &record->currentClusterOffset, record->currentEntryOffset, attr, FILEIO_SEARCH_PARTIAL_STRING_SEARCH | FILEIO_SEARCH_ENTRY_ATTRIBUTES);
    }

    if (error != FILEIO_ERROR_NONE)
    {
        directory.drive->error = error;
        return FILEIO_RESULT_FAILURE;
    }

    // Copy file data into the result structure
    FILEIO_ShortFileNameConvert ((char *)record->shortFileName, (char *)file.name);
    record->attributes = file.attributes;
    record->fileSize = file.size;
    record->timeStamp.date.value = file.date;
    record->timeStamp.time.value = file.time;
    record->timeStamp.timeMs = file.timeMs;

    record->currentEntryOffset = file.entry + 1;

    return FILEIO_RESULT_SUCCESS;
}
#endif

#if !defined (FILEIO_CONFIG_SEARCH_DISABLE)
int FILEIO_LongFileNameGet (FILEIO_SEARCH_RECORD * record, uint16_t * buffer, uint16_t length)
{
    uint32_t currentCluster = record->currentDirCluster;
    uint16_t currentClusterOffset = record->currentClusterOffset;
    uint16_t entryOffset = record->currentEntryOffset - 1;
    FILEIO_DIRECTORY_ENTRY * entry;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DIRECTORY directory;
    uint8_t checksum = 0;
    uint8_t * source;
    uint16_t i;

    directory.cluster = record->baseDirCluster;
    directory.drive = FILEIO_CharToDrive(record->driveId);

    if (directory.drive == NULL)
    {
        globalParameters.currentWorkingDirectory.drive->error = FILEIO_ERROR_INVALID_ARGUMENT;
        return FILEIO_RESULT_FAILURE;
    }

    entry = FILEIO_DirectoryEntryCache (&directory, &error, &currentCluster, &currentClusterOffset, entryOffset);
    if ((entry == NULL) || (error != FILEIO_ERROR_NONE))
    {
        directory.drive->error = error;
        return FILEIO_RESULT_FAILURE;
    }

    checksum = 0;
    source = (uint8_t *)entry->name;

    for (i = 11; i != 0; i--)
    {
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *source++;
    }

    if (FILEIO_LongFileNameCache(&directory, entryOffset, currentCluster, checksum) == FILEIO_LFN_SUCCESS)
    {
        i = FILEIO_strlen16 (lfnBuffer);
        if (i < length)
        {
            length = i;
        }

        memcpy (buffer, lfnBuffer, (length + 1) << 1);

        return FILEIO_RESULT_SUCCESS;
    }
    else
    {
        directory.drive->error = FILEIO_ERROR_NO_LONG_FILE_NAME;
        return FILEIO_RESULT_FAILURE;
    }
}
#endif


#if !defined (FILEIO_CONFIG_FORMAT_DISABLE)
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_CreateMBR (FILEIO_DRIVE_CONFIG * config, void * mediaParameters, uint32_t firstSector, uint32_t sectorCount)
{
    FILEIO_MASTER_BOOT_RECORD * partition;
    FILEIO_BUFFER_STATUS * bufferStatusPtr;
    uint8_t * dataBuffer;
    uint32_t cylHeadSec = 0x00000000;
    uint32_t tempSector;

    if ((firstSector == 0) || (sectorCount <= 1))
        return FILEIO_RESULT_FAILURE;

    if (firstSector > (sectorCount - 1))
        return FILEIO_RESULT_FAILURE;

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    bufferStatusPtr = &bufferStatus;
    dataBuffer = gDataBuffer;

    // Use the last drive's buffer for this operation (it's the least likely to be in use)
    if (bufferStatusPtr->driveOwner != NULL)
    {
        if (bufferStatusPtr->flags.dataBufferNeedsWrite)
        {
            if (! (*((const FILEIO_DRIVE_CONFIG *)(bufferStatusPtr->driveOwner))->funcSectorWrite)(mediaParameters, bufferStatusPtr->dataBufferCachedSector, dataBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.dataBufferNeedsWrite = false;
        }
    }

    bufferStatusPtr->driveOwner = NULL;
#else
    bufferStatusPtr = &bufferStatus[FILEIO_CONFIG_MAX_DRIVES - 1];
    dataBuffer = gDataBuffer[FILEIO_CONFIG_MAX_DRIVES - 1];
    // Use the last drive's buffer for this operation (it's the least likely to be in use)
    if (!gDriveSlotOpen[FILEIO_CONFIG_MAX_DRIVES - 1])
    {
        if (bufferStatusPtr->flags.dataBufferNeedsWrite)
        {
            if (! (*config->funcSectorWrite)(config->mediaParameters, bufferStatusPtr->dataBufferCachedSector, dataBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.dataBufferNeedsWrite = false;
        }
    }
#endif

    bufferStatusPtr->dataBufferCachedSector = 0xFFFFFFFF;

    memset (dataBuffer, 0x00, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

    partition = (FILEIO_MASTER_BOOT_RECORD *) dataBuffer;

    // Set Cylinder-head-sector address of the first sector
    tempSector = firstSector;
    cylHeadSec = (tempSector / (unsigned int)16065 ) << 14;
    tempSector %= 16065;
    cylHeadSec |= (tempSector / 63) << 6;
    tempSector %= 63;
    cylHeadSec |= tempSector + 1;
    *(dataBuffer + 447) = (uint8_t)((cylHeadSec >> 16) & 0xFF);
    *(dataBuffer + 448) = (uint8_t)((cylHeadSec >> 8) & 0xFF);
    *(dataBuffer + 449) = (uint8_t)((cylHeadSec) & 0xFF);

    // Set the count of sectors
    partition->partition0.sectorCount = sectorCount - firstSector;

    // Set the partition type
    // We only support creating FAT12 and FAT16 MBRs at this time
    if (partition->partition0.sectorCount < 0x1039)
    {
        // FAT12
        partition->partition0.fileSystemDescriptor = 0x01;
    }
    else if (partition->partition0.sectorCount <= 0x3FFD5F)
    {
        // FAT16
        partition->partition0.fileSystemDescriptor = 0x06;
    }
    else
    {
        return FILEIO_RESULT_FAILURE;
    }

    // Set the LBA of the first sector
    partition->partition0.lbaFirstSector = firstSector;

    // Set the Cylinder-head-sector address of the last sector
    tempSector = firstSector + sectorCount - 1;
    cylHeadSec = (tempSector / (unsigned int)16065 ) << 14;
    tempSector %= 16065;
    cylHeadSec |= (tempSector / 63) << 6;
    tempSector %= 63;
    cylHeadSec |= tempSector + 1;
    *(dataBuffer + 451) = (uint8_t)((cylHeadSec >> 16) & 0xFF);
    *(dataBuffer + 452) = (uint8_t)((cylHeadSec >> 8) & 0xFF);
    *(dataBuffer + 453) = (uint8_t)((cylHeadSec) & 0xFF);

    // Set the boot descriptor.  This will be 0, since we won't
    // be booting anything from our device probably
    partition->partition0.bootDescriptor = 0x00;

    // Set the signature codes
    partition->signature0 = 0x55;
    partition->signature1 = 0xAA;

    if (!(*config->funcSectorWrite)(mediaParameters, 0, dataBuffer, true))
    {
        return FILEIO_RESULT_FAILURE;
    }
    else
    {
        return FILEIO_RESULT_SUCCESS;
    }
}
#endif
#endif

#if !defined (FILEIO_CONFIG_FORMAT_DISABLE)
#if !defined (FILEIO_CONFIG_WRITE_DISABLE)
int FILEIO_Format (FILEIO_DRIVE_CONFIG * config, void * mediaParameters, FILEIO_FORMAT_MODE mode, uint32_t serialNumber, char * volumeId)
{
    FILEIO_MASTER_BOOT_RECORD * masterBootRecord;
    uint32_t    sectorCount, dataClusters, rootDirSectors;
    FILEIO_BOOT_SECTOR * bootSec;
    FILEIO_DRIVE d;
    FILEIO_DRIVE * disk = &d;
    uint16_t    j;
    uint32_t   fatSize, test;
    uint32_t index;
    FILEIO_MEDIA_INFORMATION * mediaInfo;
    FILEIO_BUFFER_STATUS * bufferStatusPtr;
    FILEIO_DRIVE * tempDriveOwner;

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    bufferStatusPtr = &bufferStatus;
    d.dataBuffer = gDataBuffer;
    d.fatBuffer = gFATBuffer;

    if (bufferStatusPtr->driveOwner != NULL)
    {
        FILEIO_DRIVE_CONFIG * bufferDriveConfig = (FILEIO_DRIVE_CONFIG *)((FILEIO_DRIVE *)bufferStatusPtr->driveOwner)->driveConfig;

        if (bufferStatusPtr->flags.dataBufferNeedsWrite)
        {
            if (!(*bufferDriveConfig->funcSectorWrite)(mediaParameters, bufferStatusPtr->dataBufferCachedSector, disk->dataBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.dataBufferNeedsWrite = false;
        }
        if (bufferStatusPtr->flags.fatBufferNeedsWrite)
        {
            if (! (*bufferDriveConfig->funcSectorWrite)(mediaParameters, bufferStatusPtr->fatBufferCachedSector, disk->fatBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.fatBufferNeedsWrite = false;
        }
    }
#else
    bufferStatusPtr = &bufferStatus[FILEIO_CONFIG_MAX_DRIVES - 1];
    d.dataBuffer = gDataBuffer[FILEIO_CONFIG_MAX_DRIVES - 1];
    d.fatBuffer = gFatBuffer[FILEIO_CONFIG_MAX_DRIVES - 1];

    if (!gDriveSlotOpen[FILEIO_CONFIG_MAX_DRIVES - 1])
    {
        if (bufferStatusPtr->flags.dataBufferNeedsWrite)
        {
            if (!(*config->funcSectorWrite)(mediaParameters, bufferStatusPtr->dataBufferCachedSector, disk->dataBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.dataBufferNeedsWrite = false;
        }
        if (bufferStatusPtr->flags.fatBufferNeedsWrite)
        {
            if (! (*config->funcSectorWrite)(mediaParameters, bufferStatusPtr->fatBufferCachedSector, disk->fatBuffer, false))
            {
                return false;
            }
            bufferStatusPtr->flags.fatBufferNeedsWrite = false;
        }
    }

#endif

    bufferStatusPtr->dataBufferCachedSector = 0xFFFFFFFF;
    bufferStatusPtr->fatBufferCachedSector = 0xFFFFFFFF;

    disk->bufferStatusPtr = bufferStatusPtr;
    disk->driveConfig = config;

    if(config->funcIOInit != NULL)
    {
        (*config->funcIOInit)(mediaParameters);
    }

#if defined (__XC8__)
    // XC8 cannot parse this operation without several intermediate steps
    {
        FILEIO_DRIVER_MediaInitialize funcMediaInit = config->funcMediaInit;
        void * mediaParameters = config->mediaParameters;

        mediaInfo = (*funcMediaInit)(mediaParameters);
    }
#else
    mediaInfo = (*config->funcMediaInit)(mediaParameters);
#endif
    
    if (mediaInfo->errorCode != MEDIA_NO_ERROR)
    {
        return FILEIO_RESULT_FAILURE;
    }

    if ((*config->funcSectorRead)(mediaParameters, 0x00, disk->dataBuffer) == false)
    {
        return FILEIO_RESULT_FAILURE;
    }

    // Check if the card has no MBR
    bootSec = (FILEIO_BOOT_SECTOR *) disk->dataBuffer;
    if((bootSec->signature0 == FILEIO_FAT_GOOD_SIGN_0) && (bootSec->signature1 == FILEIO_FAT_GOOD_SIGN_1))
    {
        // Technically, the OEM name is not for indication
        // The alternative is to read the CIS from attribute
        // memory.  See the PCMCIA metaformat for more details
#if defined (__XC16__) || defined (__XC32__)
        if ((*(disk->dataBuffer + BSI_FSTYPE ) == 'F') && \
            (*(disk->dataBuffer + BSI_FSTYPE + 1 ) == 'A') && \
            (*(disk->dataBuffer + BSI_FSTYPE + 2 ) == 'T') && \
            (*(disk->dataBuffer + BSI_FSTYPE + 3 ) == '1') && \
            (*(disk->dataBuffer + BSI_BOOTSIG) == 0x29))
#else
        if ((bootSec->biosParameterBlock.fat16.fileSystemType[0] == 'F') && \
            (bootSec->biosParameterBlock.fat16.fileSystemType[1] == 'A') && \
            (bootSec->biosParameterBlock.fat16.fileSystemType[2] == 'T') && \
            (bootSec->biosParameterBlock.fat16.fileSystemType[3] == '1') && \
            (bootSec->biosParameterBlock.fat16.bootSignature == 0x29))
#endif
        {
            /* Mark that we do not have a MBR;
                this is not actualy used - is here only to remove a compilation warning */
            masterBootRecord = (FILEIO_MASTER_BOOT_RECORD *) NULL;
            switch (mode)
            {
                case FILEIO_FORMAT_BOOT_SECTOR:
                    // not enough info to construct our own boot sector
                    return FILEIO_RESULT_FAILURE;
                case FILEIO_FORMAT_ERASE:
                    // We have to determine the operating system, and the
                    // locations and sizes of the root dir and FAT, and the
                    // count of FATs
                    disk->firstPartitionSector = 0;
                    tempDriveOwner = bufferStatusPtr->driveOwner;
                    bufferStatusPtr->driveOwner = disk;
                    if (FILEIO_LoadBootSector (disk) != FILEIO_ERROR_NONE)
                    {
                        bufferStatusPtr->driveOwner = tempDriveOwner;
                        return FILEIO_RESULT_FAILURE;
                    }
                    bufferStatusPtr->driveOwner = tempDriveOwner;
                default:
                    break;
            }
        }
        else
        {
            masterBootRecord = (FILEIO_MASTER_BOOT_RECORD *) disk->dataBuffer;
            disk->firstPartitionSector = masterBootRecord->partition0.lbaFirstSector;
        }
    }
    else
    {
        /* If the signature is not correct, this is neither a MBR, nor a VBR */
        return FILEIO_RESULT_FAILURE;
    }

    switch (mode)
    {
        // True: Rewrite the whole boot sector
        case FILEIO_FORMAT_BOOT_SECTOR:
            sectorCount = masterBootRecord->partition0.sectorCount;

            if (sectorCount < 0x1039)
            {
                disk->type = FILEIO_FILE_SYSTEM_TYPE_FAT12;
                // Format to FAT12 only if there are too few sectors to format
                // as FAT16
                masterBootRecord->partition0.fileSystemDescriptor = 0x01;
                if ((*config->funcSectorWrite) (mediaParameters, 0x00, disk->dataBuffer, true) == false)
                {
                    return FILEIO_RESULT_FAILURE;
                }

                if (sectorCount >= 0x1028)
                {
                    // More than 0x18 sectors for FATs, 0x20 for root dir,
                    // 0x8 reserved, and 0xFED for data
                    // So double the number of sectors in a cluster to reduce
                    // the number of data clusters used
                    disk->sectorsPerCluster = 2;
                }
                else
                {
                    // One sector per cluster
                    disk->sectorsPerCluster = 1;
                }

                // Prepare a boot sector
                memset (disk->dataBuffer, 0x00, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

                // Last digit of file system name (FAT12   )
                disk->dataBuffer[58] = '2';

                // Calculate the size of the FAT
                fatSize = (sectorCount - 0x21  + (2*disk->sectorsPerCluster));
                test =   (341 * disk->sectorsPerCluster) + 2;
                fatSize = (fatSize + (test-1)) / test;

                disk->fatCopyCount = 0x02;
                disk->rootDirectoryEntryCount = 0x200;

                disk->fatSectorCount = fatSize;
            }
            else if (sectorCount <= 0x3FFD5F)
            {
                disk->type = FILEIO_FILE_SYSTEM_TYPE_FAT16;
                // Format to FAT16
                masterBootRecord->partition0.fileSystemDescriptor = 0x06;
                if ((*config->funcSectorWrite)(mediaParameters, 0x00, disk->dataBuffer, true) == false)
                {
                    return FILEIO_RESULT_FAILURE;
                }

                dataClusters = sectorCount - 0x218;
                // Figure out how many sectors per cluster we need
                disk->sectorsPerCluster = 1;
                while (dataClusters > 0xFFED)
                {
                    disk->sectorsPerCluster *= 2;
                    dataClusters /= 2;
                }
                // This shouldnt happen
                if (disk->sectorsPerCluster > 128)
                {
                    return FILEIO_RESULT_FAILURE;
                }

                // Prepare a boot sector
                memset (disk->dataBuffer, 0x00, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

                // Last digit of file system name (FAT16   )
                disk->dataBuffer[58] = '6';

                // Calculate the size of the FAT
                fatSize = (sectorCount - 0x21  + (2*disk->sectorsPerCluster));
                test =    (256  * disk->sectorsPerCluster) + 2;
                fatSize = (fatSize + (test-1)) / test;

                disk->fatCopyCount = 0x02;
                disk->rootDirectoryEntryCount = 0x200;

                disk->fatSectorCount = fatSize;
            }
            else
            {
                disk->type = FILEIO_FILE_SYSTEM_TYPE_FAT32;
                // Format to FAT32
                masterBootRecord->partition0.fileSystemDescriptor = 0x0B;
                if ((*config->funcSectorWrite)(mediaParameters, 0x00, disk->dataBuffer, true) == false)
                {
                    return FILEIO_RESULT_FAILURE;
                }

                #ifdef FORMAT_SECTORS_PER_CLUSTER
                    disk->sectorsPerCluster = FORMAT_SECTORS_PER_CLUSTER;
                    dataClusters = sectorCount / disk->sectorsPerCluster;

                    /* FAT32: 65526 < Number of clusters < 4177918 */
                    if ((dataClusters <= 65526) || (dataClusters >= 4177918))
                    {
                        bufferStatusPtr->driveOwner = tempDriveOwner;
                        return FILEIO_RESULT_FAILURE;
                    }
                #else
                    /*  FAT32: 65526 < Number of clusters < 4177918 */
                    dataClusters = sectorCount;
                    // Figure out how many sectors per cluster we need
                    disk->sectorsPerCluster = 1;
                    while (dataClusters > 0x3FBFFE)
                    {
                        disk->sectorsPerCluster *= 2;
                        dataClusters /= 2;
                    }
                #endif
                // Check the cluster size: FAT32 supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K
                if (disk->sectorsPerCluster > 128)
                {
                    return FILEIO_RESULT_FAILURE;
                }

                // Prepare a boot sector
                memset (disk->dataBuffer, 0x00, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

               // Calculate the size of the FAT
                fatSize = (sectorCount - 0x20);
                test =    (128  * disk->sectorsPerCluster) + 1;
                fatSize = (fatSize + (test-1)) / test;

                disk->fatCopyCount = 0x02;
                disk->rootDirectoryEntryCount = 0x200;

                disk->fatSectorCount = fatSize;
            }

            // Non-file system specific values
            disk->dataBuffer[0] = 0xEB;         //Jump instruction
            disk->dataBuffer[1] = 0x3C;
            disk->dataBuffer[2] = 0x90;
            disk->dataBuffer[3] =  'M';         //OEM Name "MCHP FAT"
            disk->dataBuffer[4] =  'C';
            disk->dataBuffer[5] =  'H';
            disk->dataBuffer[6] =  'P';
            disk->dataBuffer[7] =  ' ';
            disk->dataBuffer[8] =  'F';
            disk->dataBuffer[9] =  'A';
            disk->dataBuffer[10] = 'T';

            disk->dataBuffer[11] = 0x00;             //Sector size
            disk->dataBuffer[12] = 0x02;

            disk->dataBuffer[13] = disk->sectorsPerCluster;   //Sectors per cluster

            if ((disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT12) || (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT16))
            {
                disk->dataBuffer[14] = 0x08;         //Reserved sector count
                disk->dataBuffer[15] = 0x00;
                disk->firstFatSector = 0x08 + disk->firstPartitionSector;

                disk->dataBuffer[16] = 0x02;         //number of FATs

                disk->dataBuffer[17] = 0x00;          //Max number of root directory entries - 512 files allowed
                disk->dataBuffer[18] = 0x02;

                disk->dataBuffer[19] = 0x00;         //total sectors
                disk->dataBuffer[20] = 0x00;

                disk->dataBuffer[21] = 0xF8;         //Media Descriptor

                disk->dataBuffer[22] = fatSize & 0xFF;         //Sectors per FAT
                disk->dataBuffer[23] = (fatSize >> 8) & 0xFF;

                disk->dataBuffer[24] = 0x3F;           //Sectors per track
                disk->dataBuffer[25] = 0x00;

                disk->dataBuffer[26] = 0xFF;         //Number of heads
                disk->dataBuffer[27] = 0x00;

                // Hidden sectors = sectors between the MBR and the boot sector
                disk->dataBuffer[28] = (uint8_t)(disk->firstPartitionSector & 0xFF);
                disk->dataBuffer[29] = (uint8_t)((disk->firstPartitionSector / 0x100) & 0xFF);
                disk->dataBuffer[30] = (uint8_t)((disk->firstPartitionSector / 0x10000) & 0xFF);
                disk->dataBuffer[31] = (uint8_t)((disk->firstPartitionSector / 0x1000000) & 0xFF);

                // Total Sectors = same as sectors in the partition from MBR
                disk->dataBuffer[32] = (uint8_t)(sectorCount & 0xFF);
                disk->dataBuffer[33] = (uint8_t)((sectorCount / 0x100) & 0xFF);
                disk->dataBuffer[34] = (uint8_t)((sectorCount / 0x10000) & 0xFF);
                disk->dataBuffer[35] = (uint8_t)((sectorCount / 0x1000000) & 0xFF);

                disk->dataBuffer[36] = 0x00;         // Physical drive number

                disk->dataBuffer[37] = 0x00;         // Reserved (current head)

                disk->dataBuffer[38] = 0x29;         // Signature code

                disk->dataBuffer[39] = (uint8_t)(serialNumber & 0xFF);
                disk->dataBuffer[40] = (uint8_t)((serialNumber / 0x100) & 0xFF);
                disk->dataBuffer[41] = (uint8_t)((serialNumber / 0x10000) & 0xFF);
                disk->dataBuffer[42] = (uint8_t)((serialNumber / 0x1000000) & 0xFF);

                // Volume ID
                if (volumeId != NULL)
                {
                    for (index = 0; (*(volumeId + index) != 0) && (index < 11); index++)
                    {
                        disk->dataBuffer[index + 43] = *(volumeId + index);
                    }
                    while (index < 11)
                    {
                        disk->dataBuffer[43 + index++] = 0x20;
                    }
                }
                else
                {
                    for (index = 0; index < 11; index++)
                    {
                        disk->dataBuffer[index+43] = 0;
                    }
                }

                disk->dataBuffer[54] = 'F';
                disk->dataBuffer[55] = 'A';
                disk->dataBuffer[56] = 'T';
                disk->dataBuffer[57] = '1';
                disk->dataBuffer[59] = ' ';
                disk->dataBuffer[60] = ' ';
                disk->dataBuffer[61] = ' ';

            }
            else //FAT32
            {
                disk->dataBuffer[14] = 0x20;         //Reserved sector count
                disk->dataBuffer[15] = 0x00;
                disk->firstFatSector = 0x20 + disk->firstPartitionSector;

                disk->dataBuffer[16] = 0x02;         //number of FATs

                disk->dataBuffer[17] = 0x00;          //Max number of root directory entries - 512 files allowed
                disk->dataBuffer[18] = 0x00;

                disk->dataBuffer[19] = 0x00;         //total sectors
                disk->dataBuffer[20] = 0x00;

                disk->dataBuffer[21] = 0xF8;         //Media Descriptor

                disk->dataBuffer[22] = 0x00;         //Sectors per FAT
                disk->dataBuffer[23] = 0x00;

                disk->dataBuffer[24] = 0x3F;         //Sectors per track
                disk->dataBuffer[25] = 0x00;

                disk->dataBuffer[26] = 0xFF;         //Number of heads
                disk->dataBuffer[27] = 0x00;

                // Hidden sectors = sectors between the MBR and the boot sector
                disk->dataBuffer[28] = (uint8_t)(disk->firstPartitionSector & 0xFF);
                disk->dataBuffer[29] = (uint8_t)((disk->firstPartitionSector / 0x100) & 0xFF);
                disk->dataBuffer[30] = (uint8_t)((disk->firstPartitionSector / 0x10000) & 0xFF);
                disk->dataBuffer[31] = (uint8_t)((disk->firstPartitionSector / 0x1000000) & 0xFF);

                // Total Sectors = same as sectors in the partition from MBR
                disk->dataBuffer[32] = (uint8_t)(sectorCount & 0xFF);
                disk->dataBuffer[33] = (uint8_t)((sectorCount / 0x100) & 0xFF);
                disk->dataBuffer[34] = (uint8_t)((sectorCount / 0x10000) & 0xFF);
                disk->dataBuffer[35] = (uint8_t)((sectorCount / 0x1000000) & 0xFF);

                disk->dataBuffer[36] = fatSize & 0xFF;         //Sectors per FAT
                disk->dataBuffer[37] = (fatSize >>  8) & 0xFF;
                disk->dataBuffer[38] = (fatSize >> 16) & 0xFF;
                disk->dataBuffer[39] = (fatSize >> 24) & 0xFF;

                disk->dataBuffer[40] = 0x00;         //Active FAT
                disk->dataBuffer[41] = 0x00;

                disk->dataBuffer[42] = 0x00;         //File System version
                disk->dataBuffer[43] = 0x00;

                disk->dataBuffer[44] = 0x02;         //First cluster of the root directory
                disk->dataBuffer[45] = 0x00;
                disk->dataBuffer[46] = 0x00;
                disk->dataBuffer[47] = 0x00;

                disk->dataBuffer[48] = 0x01;         //FSInfo
                disk->dataBuffer[49] = 0x00;

                disk->dataBuffer[50] = 0x00;         //Backup Boot Sector
                disk->dataBuffer[51] = 0x00;

                disk->dataBuffer[52] = 0x00;         //Reserved for future expansion
                disk->dataBuffer[53] = 0x00;
                disk->dataBuffer[54] = 0x00;
                disk->dataBuffer[55] = 0x00;
                disk->dataBuffer[56] = 0x00;
                disk->dataBuffer[57] = 0x00;
                disk->dataBuffer[58] = 0x00;
                disk->dataBuffer[59] = 0x00;
                disk->dataBuffer[60] = 0x00;
                disk->dataBuffer[61] = 0x00;
                disk->dataBuffer[62] = 0x00;
                disk->dataBuffer[63] = 0x00;

                disk->dataBuffer[64] = 0x00;         // Physical drive number

                disk->dataBuffer[65] = 0x00;         // Reserved (current head)

                disk->dataBuffer[66] = 0x29;         // Signature code

                disk->dataBuffer[67] = (uint8_t)(serialNumber & 0xFF);
                disk->dataBuffer[68] = (uint8_t)((serialNumber / 0x100) & 0xFF);
                disk->dataBuffer[69] = (uint8_t)((serialNumber / 0x10000) & 0xFF);
                disk->dataBuffer[70] = (uint8_t)((serialNumber / 0x1000000) & 0xFF);

                // Volume ID
                if (volumeId != NULL)
                {
                    for (index = 0; (*(volumeId + index) != 0) && (index < 11); index++)
                    {
                        disk->dataBuffer[index + 71] = *(volumeId + index);
                    }
                    while (index < 11)
                    {
                        disk->dataBuffer[71 + index++] = 0x20;
                    }
                }
                else
                {
                    memset (disk->dataBuffer, 0x00, 11);
                }

                disk->dataBuffer[82] = 'F';
                disk->dataBuffer[83] = 'A';
                disk->dataBuffer[84] = 'T';
                disk->dataBuffer[85] = '3';
                disk->dataBuffer[86] = '2';
                disk->dataBuffer[87] = ' ';
                disk->dataBuffer[88] = ' ';
                disk->dataBuffer[89] = ' ';


            }

#ifdef __XC8__
            // C18 can't reference a value greater than 256
            // using an array name pointer
            *(dataBufferPointer + 510) = 0x55;
            *(dataBufferPointer + 511) = 0xAA;
#else
            disk->dataBuffer[510] = 0x55;
            disk->dataBuffer[511] = 0xAA;
#endif

            disk->firstRootSector = disk->firstFatSector + (disk->fatCopyCount * disk->fatSectorCount);

            if ((*config->funcSectorWrite) (mediaParameters, disk->firstPartitionSector, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }

            break;
        case FILEIO_FORMAT_ERASE:
            tempDriveOwner = bufferStatusPtr->driveOwner;
            bufferStatusPtr->driveOwner = disk;
            if (FILEIO_LoadBootSector (disk) != FILEIO_ERROR_NONE)
            {
                bufferStatusPtr->driveOwner = tempDriveOwner;
                return FILEIO_RESULT_FAILURE;
            }
            bufferStatusPtr->driveOwner = tempDriveOwner;
            break;
        default:
            return FILEIO_RESULT_FAILURE;
    }

    // Erase the FAT
    memset (disk->dataBuffer, 0x00, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

    if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT32)
    {
        disk->dataBuffer[0] = 0xF8;          //BPB_Media byte value in its low 8 bits, and all other bits are set to 1
        disk->dataBuffer[1] = 0xFF;
        disk->dataBuffer[2] = 0xFF;
        disk->dataBuffer[3] = 0xFF;

        disk->dataBuffer[4] = 0x00;          //Disk is clean and no read/write errors were encountered
        disk->dataBuffer[5] = 0x00;
        disk->dataBuffer[6] = 0x00;
        disk->dataBuffer[7] = 0x0C;

        disk->dataBuffer[8]  = 0xFF;         //Root Directory EOF
        disk->dataBuffer[9]  = 0xFF;
        disk->dataBuffer[10] = 0xFF;
        disk->dataBuffer[11] = 0xFF;

        for (j = disk->fatCopyCount - 1; j != 0xFFFF; j--)
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstFatSector + (j * disk->fatSectorCount), disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        memset (disk->dataBuffer, 0x00, 12);

        for (index = disk->firstFatSector + 1; index < (disk->firstFatSector + disk->fatSectorCount); index++)
        {
            for (j = disk->fatCopyCount - 1; j != 0xFFFF; j--)
            {
                if ((*config->funcSectorWrite)(mediaParameters, index + (j * disk->fatSectorCount), disk->dataBuffer, false) == false)
                {
                    return FILEIO_RESULT_FAILURE;
                }
            }
        }

        // Erase the root directory
        for (index = 1; index < disk->sectorsPerCluster; index++)
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector + index, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        if (volumeId != NULL)
        {
            // Create a drive name entry in the root dir
            index = 0;
            while ((*(volumeId + index) != 0) && (index < 11))
            {
                disk->dataBuffer[index] = *(volumeId + index);
                index++;
            }
            while (index < 11)
            {
                disk->dataBuffer[index++] = ' ';
            }
            disk->dataBuffer[11] = 0x08;
            disk->dataBuffer[17] = 0x11;
            disk->dataBuffer[19] = 0x11;
            disk->dataBuffer[23] = 0x11;

            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }
        else
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        return FILEIO_RESULT_SUCCESS;
    }
    else
    {
        disk->dataBuffer[0] = 0xF8;
        disk->dataBuffer[1] = 0xFF;
        disk->dataBuffer[2] = 0xFF;
        if (disk->type == FILEIO_FILE_SYSTEM_TYPE_FAT16)
        {
            disk->dataBuffer[3] = 0xFF;
        }

        for (j = disk->fatCopyCount - 1; j != 0xFFFF; j--)
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstFatSector + (j * disk->fatSectorCount), disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        memset (disk->dataBuffer, 0x00, 4);

        for (index = disk->firstFatSector + 1; index < (disk->firstFatSector + disk->fatSectorCount); index++)
        {
            for (j = disk->fatCopyCount - 1; j != 0xFFFF; j--)
            {
                if ((*config->funcSectorWrite)(mediaParameters, index + (j * disk->fatSectorCount), disk->dataBuffer, false) == false)
                {
                    return FILEIO_RESULT_FAILURE;
                }
            }
        }

		// Initialize the sector size
        disk->sectorSize = FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

        // Erase the root directory
        rootDirSectors = ((disk->rootDirectoryEntryCount * 32) + (disk->sectorSize - 1)) / disk->sectorSize;

        for (index = 1; index < rootDirSectors; index++)
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector + index, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        if (volumeId != NULL)
        {
            // Create a drive name entry in the root dir
            index = 0;
            while ((*(volumeId + index) != 0) && (index < 11))
            {
                disk->dataBuffer[index] = *(volumeId + index);
                index++;
            }
            while (index < 11)
            {
                disk->dataBuffer[index++] = ' ';
            }
            disk->dataBuffer[11] = 0x08;
            disk->dataBuffer[17] = 0x11;
            disk->dataBuffer[19] = 0x11;
            disk->dataBuffer[23] = 0x11;

            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }
        else
        {
            if ((*config->funcSectorWrite)(mediaParameters, disk->firstRootSector, disk->dataBuffer, false) == false)
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        return FILEIO_RESULT_SUCCESS;
    }
}
#endif
#endif

#if !defined (FILEIO_CONFIG_DRIVE_PROPERTIES_DISABLE)
void FILEIO_DrivePropertiesGet(FILEIO_DRIVE_PROPERTIES * properties, uint16_t driveId)
{
    uint8_t i;
    uint32_t value = 0x0;
    FILEIO_DRIVE * drive;

    drive = FILEIO_CharToDrive (driveId);
    if (drive == NULL)
    {
        properties->properties_status = FILEIO_GET_PROPERTIES_DRIVE_NOT_MOUNTED;
        return;
    }

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
    if (FILEIO_GetSingleBuffer (drive) != FILEIO_RESULT_SUCCESS)
    {
        properties->properties_status = FILEIO_GET_PROPERTIES_CACHE_ERROR;
        return;
    }
#endif

    if(properties->new_request == true)
    {
        properties->results.free_clusters = 0;
        properties->new_request = false;

        properties->properties_status = FILEIO_GET_PROPERTIES_STILL_WORKING;

        properties->results.disk_format = drive->type;
        properties->results.sector_size = drive->sectorSize;
        properties->results.sectors_per_cluster = drive->sectorsPerCluster;
        properties->results.total_clusters = drive->partitionClusterCount;

        /* Settings based on FAT type */
        switch (drive->type)
        {
            case FILEIO_FILE_SYSTEM_TYPE_FAT32:
                properties->private.EndClusterLimit = FILEIO_CLUSTER_VALUE_FAT32_END;
                properties->private.ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT32_FAIL;
                break;
            case FILEIO_FILE_SYSTEM_TYPE_FAT16:
                properties->private.EndClusterLimit = FILEIO_CLUSTER_VALUE_FAT16_END;
                properties->private.ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
                break;
            case FILEIO_FILE_SYSTEM_TYPE_FAT12:
                properties->private.EndClusterLimit = FILEIO_CLUSTER_VALUE_FAT12_END;
                properties->private.ClusterFailValue = FILEIO_CLUSTER_VALUE_FAT16_FAIL;
                break;
        }

        properties->private.c = 2;

        properties->private.curcls = properties->private.c;
        FILEIO_FATRead(drive, properties->private.c);
    }

    if(properties->properties_status != FILEIO_GET_PROPERTIES_STILL_WORKING)
    {
        return;
    }

    // sequentially scan through the FAT looking for an empty cluster
    for(i=0;i<255;i++)
    {
        // look at its value
        if ( (value = FILEIO_FATRead(drive, properties->private.c)) == properties->private.ClusterFailValue)
        {
            properties->properties_status = FILEIO_GET_PROPERTIES_CLUSTER_FAILURE;
            return;
        }

        // check if empty cluster found
        if (value == FILEIO_CLUSTER_VALUE_EMPTY)
        {
            properties->results.free_clusters++;
        }

        properties->private.c++;    // check next cluster in FAT
        // check if reached last cluster in FAT, re-start from top
        if ((value == properties->private.EndClusterLimit) || (properties->private.c >= (properties->results.total_clusters + 2)))
            properties->private.c = 2;

        // check if full circle done, disk full
        if ( properties->private.c == properties->private.curcls)
        {
            properties->properties_status = FILEIO_GET_PROPERTIES_NO_ERRORS;
            return;
        }
    }  // scanning for an empty cluster

    properties->properties_status = FILEIO_GET_PROPERTIES_STILL_WORKING;
    return;
}
#endif

void FILEIO_ShortFileNameConvert (char * newFileName, char * oldFileName)
{
    uint8_t j = 0;
    while (j < 8)
    {
        if (*oldFileName == 0x20)
        {
            break;
        }
        *newFileName++ = *oldFileName++;
        j++;
    }

    while (j < 8)
    {
        oldFileName++;
        j++;
    }

    if (*oldFileName != 0x20)
    {
        *newFileName++ = '.';
        while (j < 11)
        {
            if (*oldFileName == 0x20)
            {
                break;
            }
            *newFileName++ = *oldFileName++;
            j++;
        }
    }
    *newFileName = 0;
}

#if defined (FILEIO_CONFIG_MULTIPLE_BUFFER_MODE_DISABLE)
int FILEIO_GetSingleBuffer (FILEIO_DRIVE * drive)
{
    if (drive->bufferStatusPtr->driveOwner != drive)
    {
        if (drive != NULL)
        {
            if (!FILEIO_FlushBuffer(drive, FILEIO_BUFFER_FAT))
            {
                return FILEIO_RESULT_FAILURE;
            }
            if (!FILEIO_FlushBuffer(drive, FILEIO_BUFFER_DATA))
            {
                return FILEIO_RESULT_FAILURE;
            }
        }

        drive->bufferStatusPtr->driveOwner = drive;
        drive->bufferStatusPtr->dataBufferCachedSector = 0xFFFFFFFF;
        drive->bufferStatusPtr->fatBufferCachedSector = 0xFFFFFFFF;
    }

    return FILEIO_RESULT_SUCCESS;
}
#endif

FILEIO_ERROR_TYPE FILEIO_FindLongFileName (FILEIO_DIRECTORY * directory, FILEIO_OBJECT * filePtr, uint32_t * currentCluster, uint16_t * currentClusterOffset, uint16_t entryOffset, uint16_t attributes, FILEIO_SEARCH_TYPE mode)
{
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    FILEIO_DIRECTORY_ENTRY * entry;
    uint8_t checksum, i;
    uint8_t * source;
    uint32_t currentClusterTemp;
    uint16_t currentClusterOffsetTemp;

    if (*currentCluster == 0)
    {
        *currentCluster = directory->drive->firstRootCluster;
    }
    
    while(1)
    {
        do
        {
            entry = FILEIO_DirectoryEntryCache (directory, &error, currentCluster, currentClusterOffset, entryOffset);
            if (error == FILEIO_ERROR_DONE)
            {
                break;
            }
            else if (error == FILEIO_ERROR_BAD_CACHE_READ)
            {
                directory->drive->error = FILEIO_ERROR_BAD_CACHE_READ;
                return error;
            }

            entryOffset++;
        } while (((entry->attributes == FILEIO_ATTRIBUTE_LONG_NAME) || (entry->attributes == FILEIO_ATTRIBUTE_VOLUME) || (((uint8_t)entry->name[0]) == FILEIO_DIRECTORY_ENTRY_DELETED)) && (entry->name[0] != FILEIO_DIRECTORY_ENTRY_EMPTY));

        if ((error == FILEIO_ERROR_DONE) || (entry->name[0] == FILEIO_DIRECTORY_ENTRY_EMPTY))
        {
            return FILEIO_ERROR_DONE;
        }

        currentClusterTemp = *currentCluster;
        currentClusterOffsetTemp = *currentClusterOffset;

        // Valid file entry was found
        if (((mode & FILEIO_SEARCH_ENTRY_ATTRIBUTES) != FILEIO_SEARCH_ENTRY_ATTRIBUTES) || ((entry->attributes & attributes) == entry->attributes))
        {
            checksum = 0;
            source = (uint8_t *)entry->name;

            for (i = 11; i != 0; i--)
            {
                checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + *source++;
            }

            if (FILEIO_LongFileNameCache(directory, entryOffset - 1, *currentCluster, checksum) == FILEIO_LFN_SUCCESS)
            {
                // File's attributes are valid or we aren't trying to match attributes
                if (FILEIO_LongFileNameCompare (filePtr->lfnPtr, mode) == true)
                {
                    // Recache the short file name entry, just in case the LFN entry spans a cluster boundary
                    entry = FILEIO_DirectoryEntryCache (directory, &error, &currentClusterTemp, &currentClusterOffsetTemp, entryOffset - 1);

                    // Found a match.  Fill the result object with the file data
                    memcpy (filePtr->name, entry->name, FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX);
                    filePtr->disk = directory->drive;
                    filePtr->firstCluster = FILEIO_FullClusterNumberGet (entry);
                    filePtr->currentCluster = filePtr->firstCluster;
                    filePtr->currentSector = 0;
                    filePtr->currentOffset = 0;
                    filePtr->absoluteOffset = 0;
                    filePtr->size = entry->fileSize;
                    filePtr->attributes = entry->attributes;
                    if ((filePtr->attributes & FILEIO_ATTRIBUTE_DIRECTORY) == FILEIO_ATTRIBUTE_DIRECTORY)
                    {
                        filePtr->timeMs = entry->createTimeMs;
                        filePtr->time = entry->createTime;
                        filePtr->date = entry->createDate;
                    }
                    else
                    {
                        filePtr->time = entry->writeTime;
                        filePtr->date = entry->writeDate;
                    }
                    filePtr->entry = --entryOffset;
                    filePtr->baseClusterDir = directory->cluster;
                    filePtr->currentClusterDir = directory->cluster;

                    return FILEIO_ERROR_NONE;
                }
            }
        }
    }
}

FILEIO_LFN_ERROR FILEIO_LongFileNameCache (FILEIO_DIRECTORY * directory, uint16_t shortEntryOffset, uint32_t currentCluster, uint8_t checksum)
{
    uint16_t i = 0;
    FILEIO_DIRECTORY_ENTRY_LFN * lfnEntry;
    uint16_t entryOffset;
    FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
    uint16_t currentClusterOffset = shortEntryOffset / FILEIO_DIRECTORY_ENTRIES_PER_SECTOR;
    currentClusterOffset /= directory->drive->sectorsPerCluster;

    if (shortEntryOffset == 0)
    {
        return FILEIO_LFN_NONE;
    }

    entryOffset = shortEntryOffset - 1;

    lfnEntry = (FILEIO_DIRECTORY_ENTRY_LFN *)FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, entryOffset);

    if ((lfnEntry->attributes != FILEIO_ATTRIBUTE_LONG_NAME) || (lfnEntry->checksum != checksum))
    {
        return FILEIO_LFN_NONE;
    }

    while ((error == FILEIO_ERROR_NONE) && (lfnEntry->attributes == FILEIO_ATTRIBUTE_LONG_NAME) && (i < 256) && ((lfnEntry->sequenceNumber & 0x40) != 0x40) && (lfnEntry->checksum == checksum))
    {
        entryOffset--;

        // Copy file data to lfnData array

        memcpy (lfnBuffer + i, &lfnEntry->namePart1, 10);
        i+= 5;
        memcpy (lfnBuffer + i, &lfnEntry->namePart2, 12);
        i += 6;
        memcpy (lfnBuffer + i, &lfnEntry->namePart3, 4);
        i += 2;

        lfnEntry = (FILEIO_DIRECTORY_ENTRY_LFN *)FILEIO_DirectoryEntryCache (directory, &error, &currentCluster, &currentClusterOffset, entryOffset);
    }

    if ((error == FILEIO_ERROR_NONE) && (lfnEntry->attributes == FILEIO_ATTRIBUTE_LONG_NAME) && (i < 256) && (lfnEntry->checksum == checksum))
    {
        memcpy (lfnBuffer + i, &lfnEntry->namePart1, 10);
        i+= 5;
        memcpy (lfnBuffer + i, &lfnEntry->namePart2, 12);
        i += 6;
        memcpy (lfnBuffer + i, &lfnEntry->namePart3, 4);
        i += 2;
    }
    else
    {
        if (error == FILEIO_ERROR_NONE)
        {
            error = FILEIO_ERROR_BAD_CACHE_READ;
        }
    }

    if (error == FILEIO_ERROR_NONE)
    {
        *(lfnBuffer + i) = 0x0000;
        return FILEIO_LFN_SUCCESS;
    }
    else
    {
        return FILEIO_LFN_FAILURE;
    }
}

bool FILEIO_LongFileNameCompare (uint16_t * fileName, FILEIO_SEARCH_TYPE mode)
{
    uint16_t nameLen;

    nameLen = FILEIO_lfnlen (fileName);

    if (nameLen != FILEIO_strlen16 (lfnBuffer))
    {
        return false;
    }

    if ((mode & FILEIO_SEARCH_PARTIAL_STRING_SEARCH) == FILEIO_SEARCH_PARTIAL_STRING_SEARCH)
    {
        uint16_t character;
        uint16_t i = 0;

        while(nameLen-- != 0)
        {
            character = fileName[i];
            if (character == '*')
                break;
            if (character != '?')
            {
                if (character != lfnBuffer[i])
                {
                    return false;
                }
            }
            i++;
        }

        return true;
    }
    else
    {
        if (FILEIO_memcmp16(fileName, lfnBuffer, nameLen) == 0)
        {
            return true;
        }
    }

    return false;
}

uint16_t FILEIO_strlen16 (uint16_t * name)
{
    uint16_t i = 0;

    while (*name++ != 0)
    {
        i++;
    }

    return i;
}

uint16_t FILEIO_lfnlen (uint16_t * name)
{
    uint16_t i = 0;
    uint16_t c;

    while (((c = *name++) != 0) && (c != FILEIO_CONFIG_DELIMITER))
    {
        i++;
    }

    return i;
}

int FILEIO_memcmp16 (uint16_t * name1, uint16_t * name2, uint16_t len)
{
    while (len--)
    {
        if (*name1 != *name2)
        {
            return (*name1 - *name2);
        }
        name1++;
        name2++;
    }

    return 0;
}

uint16_t ReadRam16bit (uint8_t * pBuffer, uint16_t index)
{
    uint16_t res;

    pBuffer += index + 1;
    res = *pBuffer--;
    res <<= 8;
    res |= *pBuffer;
    return res;
}

uint32_t ReadRam32bit(uint8_t * pBuffer, uint16_t index)
{
    uint32_t res;

    pBuffer += index + 3;
    res = *pBuffer--;
    res <<= 8;
    res |= *pBuffer--;
    res <<= 8;
    res |= *pBuffer--;
    res <<= 8;
    res |= *pBuffer;

    return res;
}
