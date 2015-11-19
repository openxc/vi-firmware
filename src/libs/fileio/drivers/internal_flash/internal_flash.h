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

#include "fileio_config.h"
#include <fileio.h>

uint8_t FILEIO_InternalFlash_MediaDetect(void* config);
FILEIO_MEDIA_INFORMATION * FILEIO_InternalFlash_MediaInitialize(void* config);
uint8_t FILEIO_InternalFlash_SectorRead(void* config, uint32_t sector_addr, uint8_t* buffer);
uint8_t FILEIO_InternalFlash_SectorWrite(void* config, uint32_t sector_addr, uint8_t* buffer, uint8_t allowWriteToZero);
uint16_t FILEIO_InternalFlash_SectorSizeRead(void* config);
uint32_t FILEIO_InternalFlash_CapacityRead(void* config);
uint8_t FILEIO_InternalFlash_WriteProtectStateGet(void* config);

#if !defined(DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT)
    #define DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT 16
#endif

//Note: If only 1 FAT sector is used, assuming 12-bit (1.5 uint8_t) FAT entry size
//(ex: FAT12 filesystem), then the total FAT entries that can fit in a single 512 
//uint8_t FAT sector is (512 uint8_ts) / (1.5 uint8_ts/entry) = 341 entries.  This allows
//the FAT table to reference up to 341*512 = ~174kB of space.  Therfore, more 
//FAT sectors are needed if creating an MSD volume bigger than this.
#define DRV_FILEIO_INTERNAL_FLASH_NUM_RESERVED_SECTORS 1
#define DRV_FILEIO_INTERNAL_FLASH_NUM_VBR_SECTORS 1
#define DRV_FILEIO_INTERNAL_FLASH_NUM_FAT_SECTORS 1
#define DRV_FILEIO_INTERNAL_FLASH_NUM_ROOT_DIRECTORY_SECTORS ((DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT+15)/16) //+15 because the compiler truncates
#define DRV_FILEIO_INTERNAL_FLASH_OVERHEAD_SECTORS (\
            DRV_FILEIO_INTERNAL_FLASH_NUM_RESERVED_SECTORS + \
            DRV_FILEIO_INTERNAL_FLASH_NUM_VBR_SECTORS + \
            DRV_FILEIO_INTERNAL_FLASH_NUM_ROOT_DIRECTORY_SECTORS + \
            DRV_FILEIO_INTERNAL_FLASH_NUM_FAT_SECTORS)
#define DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE (\
            DRV_FILEIO_INTERNAL_FLASH_OVERHEAD_SECTORS + \
            DRV_FILEIO_INTERNAL_FLASH_CONFIG_DRIVE_CAPACITY)
#define DRV_FILEIO_INTERNAL_FLASH_PARTITION_SIZE (uint32_t)(DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE - 1)  //-1 is to exclude the sector used for the MBR


//---------------------------------------------------------
//Do some build time error checking
//---------------------------------------------------------
#if defined(__C30__)
    #if(DRV_FILEIO_INTERNAL_FLASH_TOTAL_DISK_SIZE % 2)
        #warning "MSD volume overlaps flash erase page with firmware program memory.  Please change your configuration settings to ensure the MSD volume cannot share an erase page with the firmware."
        //See code comments in FSconfig.h, and adjust the MDD_INTERNAL_FLASH_DRIVE_CAPACITY definition until the warning goes away.
    #endif
#endif

#if (DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT>64)
    #if defined(__C30__)
        #error "PSV only allows 32KB of memory.  The drive options selected result in more than 32KB of data.  Please reduce the number of root directory entries possible"
    #endif
#endif

#if (FILEIO_CONFIG_MEDIA_SECTOR_SIZE != 512)
    #error "The current implementation of internal flash MDD only supports a media sector size of 512.  Please modify your selected value in the FSconfig.h file."
#endif

#if (DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT != 16) && \
    (DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT != 32) && \
    (DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT != 48) && \
    (DRV_FILEIO_CONFIG_INTERNAL_FLASH_MAX_NUM_FILES_IN_ROOT != 64)
    #error "Number of root file entries must be a multiple of 16.  Please adjust the definition in the FSconfig.h file."
#endif

