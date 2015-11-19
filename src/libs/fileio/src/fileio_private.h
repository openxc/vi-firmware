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

#ifndef  _FSDEF__H
#define  _FSDEF__H

#include <stdint.h>
#include <stdbool.h>
#include <fileio.h>

// Private search paramters
typedef enum
{
    FILEIO_SEARCH_ENTRY_EMPTY = 0x01,
    FILEIO_SEARCH_ENTRY_MATCH = 0x02,
    FILEIO_SEARCH_PARTIAL_STRING_SEARCH = 0x04,
    FILEIO_SEARCH_ENTRY_ATTRIBUTES = 0x08
} FILEIO_SEARCH_TYPE;

typedef enum
{
    FILEIO_NAME_INVALID,
    FILEIO_NAME_SHORT,
    FILEIO_NAME_LONG,
    FILEIO_NAME_DOT
} FILEIO_NAME_TYPE;

typedef enum
{
    FILEIO_BUFFER_DATA,
    FILEIO_BUFFER_FAT
} FILEIO_BUFFER_ID;

#define FILEIO_DIRECTORY_ENTRIES_PER_SECTOR     0x0f        // Mask for the number of directory entries in a sector
#define FILEIO_DIRECTORY_ENTRY_SIZE             32          // Directory entry size, in bytes
#define FILEIO_DIRECTORY_ENTRY_EMPTY            0           // Value to indicate that a directory entry is empty
#define FILEIO_DIRECTORY_ENTRY_DELETED          0xE5        // Value to indicate that a directory entry has been deleted

#define FILEIO_FIXED_ROOT_DIRECTORY_CLUSTER_NUMBER      0   // Value of the root directory cluster for non-FAT32 file systems

#define FILEIO_CLUSTER_VALUE_EMPTY          0x0000          // FAT entry for an empty cluster
#define FILEIO_CLUSTER_VALUE_FAT12_EOF      0xff8           // End-of-file cluster value for FAT12
#define FILEIO_CLUSTER_VALUE_FAT16_EOF      0xfff8          // End-of-file cluster value for FAT16
#define FILEIO_CLUSTER_VALUE_FAT32_EOF      0x0ffffff8      // End-of-file cluster value for FAT32
#define FILEIO_CLUSTER_VALUE_FAT12_FAIL     0xfff           // Return value indicating failure for FAT12
#define FILEIO_CLUSTER_VALUE_FAT16_FAIL     0xffff          // Return value indicating failure for FAT16
#define FILEIO_CLUSTER_VALUE_FAT32_FAIL     0x0fffffff      // Return value indicating failure for FAT32
#define FILEIO_CLUSTER_VALUE_FAT12_END      0xff7           // Comparison value to determine if the firmware has reached the last allocatable cluster for FAT12
#define FILEIO_CLUSTER_VALUE_FAT16_END      0xfff7          // Comparison value to determine if the firmware has reached the last allocatable cluster for FAT16
#define FILEIO_CLUSTER_VALUE_FAT32_END      0x0ffffff7      // Comparison value to determine if the firmware has reached the last allocatable cluster for FAT32

#define FILEIO_MEDIA_SECTOR_MBR         0ul         // Master Boot Record sector number

#define FILEIO_FAT_GOOD_SIGN_0          0x55        // FAT signature byte 0
#define FILEIO_FAT_GOOD_SIGN_1          0xAA        // FAT signatury byte 1

typedef struct
{
    uint32_t dataBufferCachedSector;
    uint32_t fatBufferCachedSector;
    struct
    {
        unsigned dataBufferNeedsWrite : 1;
        unsigned fatBufferNeedsWrite : 1;
    } flags;
    void * driveOwner;
} FILEIO_BUFFER_STATUS;

// Structure containing information about a device
typedef struct
{
    uint32_t    firstPartitionSector;       // Logical block address of the first sector of the FAT partition on the device
    uint32_t    firstFatSector;             // Logical block address of the FAT
    uint32_t    firstRootSector;            // Logical block address of the root directory
    uint32_t    firstRootCluster;           // Cluster number of the root directory
    uint32_t    firstDataSector;            // Logical block address of the data section of the device.
    uint32_t    partitionClusterCount;      // The maximum number of clusters in the partition.
    uint32_t    sectorSize;                 // The size of a sector in bytes
    uint32_t    fatSectorCount;             // The number of sectors in the FAT
    uint8_t *   dataBuffer;                 // Address of the global data buffer used to read and write file information
    uint8_t *   fatBuffer;                  // Address of the fat buffer used to read and write sectors of the FAT
    FILEIO_BUFFER_STATUS * bufferStatusPtr;     // Pointer to a buffer status structure
    const FILEIO_DRIVE_CONFIG * driveConfig;    // Configuration information for the drive
    void *      mediaParameters;            // Parameters that describe which instance of the media to use (see [media].h for more information).
    uint16_t    rootDirectoryEntryCount;    // The maximum number of entries in the root directory.
    uint8_t     fatCopyCount;               // The number of copies of the FAT in the partition
    uint8_t     sectorsPerCluster;          // The number of sectors per cluster in the data region
    uint8_t     type;                       // The file system type of the partition (FAT12, FAT16 or FAT32)
    uint8_t     mount;                      // Device mount flag (true if disk was mounted successfully, false otherwise)
    uint8_t     error;                      // Last error that occured for this drive
    char        driveId;
#if defined __XC32__ || defined __XC16__
} __attribute__ ((packed)) FILEIO_DRIVE;
#else
} FILEIO_DRIVE;
#endif

typedef struct
{
    uint32_t cluster;
    FILEIO_DRIVE * drive;
} FILEIO_DIRECTORY;

// Directory entry structure
typedef struct
{
    char name[FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX];        // File name and extension
    uint8_t attributes;                                     // File attributes
    uint8_t reserved0;                                      // Reserved byte
    uint8_t createTimeMs;                                   // Create time (millisecond field)
    uint16_t createTime;                                    // Create time (second, minute, hour field)
    uint16_t createDate;                                    // Create date
    uint16_t accessDate;                                    // Last access date
    uint16_t firstClusterHigh;                              // High word of the entry's first cluster number
    uint16_t writeTime;                                     // Last update time
    uint16_t writeDate;                                     // Last update date
    uint16_t firstClusterLow;                               // Low word of the entry's first cluster number
    uint32_t fileSize;                                      // The 32-bit file size
} FILEIO_DIRECTORY_ENTRY;

// Long File Name Entry
typedef struct
{
   uint8_t sequenceNumber;   // Sequence number,
   uint8_t namePart1[10];    // File name part 1
   uint8_t attributes;    // File attribute
   uint8_t type;		// LFN Type
   uint8_t checksum;     // Checksum
   uint16_t namePart2[6];    // File name part 2
   uint16_t reserved0;	// Reserved for future use
   uint16_t namePart3[2];     // File name part 3
} FILEIO_DIRECTORY_ENTRY_LFN;

// BIOS Parameter Block for a FAT12 partition
typedef struct {
    uint8_t jumpCommand[3];                 // Jump Command
    uint8_t oemName[8];                     // OEM name
    uint16_t bytesPerSector;                // Number of bytes per sector
    uint8_t sectorsPerCluster;              // Number of sectors per cluster
    uint16_t reservedSectorCount;           // Number of reserved sectors at the beginning of the partition
    uint8_t fatCount;                       // Number of FATs on the partition
    uint16_t rootDirectoryEntryCount;       // Number of root directory entries
    uint16_t totalSectors;                  // Total number of sectors
    uint8_t mediaDescriptor;                // Media descriptor
    uint16_t sectorsPerFat;                 // Number of sectors per FAT
    uint16_t sectorsPerTrack;               // Number of sectors per track
    uint16_t headCount;                     // Number of heads
    uint32_t hiddenSectorCount;             // Number of hidden sectors
    uint32_t reserved0;                     // Reserved space
    uint8_t driveNumber;                    // Drive number
    uint8_t reserved1;                      // Reserved space
    uint8_t bootSignature;                  // Boot signature - equal to 0x29
    uint8_t volumeId[4];                    // Volume ID
    uint8_t volLabel[11];                   // Volume Label
    uint8_t fileSystemType[8];              // File system type in ASCII. Not used for determination
#if defined __XC32__ || defined __XC16__
} __attribute__ ((packed)) FILEIO_BIOS_PARAMETER_BLOCK_FAT12;
#else
} FILEIO_BIOS_PARAMETER_BLOCK_FAT12;
#endif

// BIOS Parameter Block for a FAT16 partition
typedef struct {
    uint8_t jumpCommand[3];                 // Jump Command
    uint8_t oemName[8];                     // OEM name
    uint16_t bytesPerSector;                // Number of bytes per sector
    uint8_t sectorsPerCluster;              // Number of sectors per cluster
    uint16_t reservedSectorCount;           // Number of reserved sectors at the beginning of the partition
    uint8_t fatCount;                       // Number of FATs on the partition
    uint16_t rootDirectoryEntryCount;       // Number of root directory entries
    uint16_t totalSectors16;                // Total number of sectors
    uint8_t mediaDescriptor;                // Media descriptor
    uint16_t sectorsPerFat;                 // Number of sectors per FAT
    uint16_t sectorsPerTrack;               // Number of sectors per track
    uint16_t headCount;                     // Number of heads
    uint32_t hiddenSectorCount;             // Number of hidden sectors
    uint32_t totalSectors32;                // Total sector count (32 bits)
    uint8_t driveNumber;                    // Drive number
    uint8_t reserved0;                      // Reserved space
    uint8_t bootSignature;                  // Boot signature - equal to 0x29
    uint8_t volumeId[4];                    // Volume ID
    uint8_t volumeLabel[11];                // Volume Label
    uint8_t fileSystemType[8];              // File system type in ASCII. Not used for determination
#if defined __XC32__ || defined __XC16__
} __attribute__ ((packed)) FILEIO_BIOS_PARAMETER_BLOCK_FAT16;
#else
} FILEIO_BIOS_PARAMETER_BLOCK_FAT16;
#endif

// BIOS Parameter Block for a FAT32 parition
typedef struct {
    uint8_t jumpCommand[3];                 // Jump Command
    uint8_t oemName[8];                     // OEM name
    uint16_t bytesPerSector;                // Number of uint8_ts per sector
    uint8_t sectorsPerCluster;              // Number of sectors per cluster
    uint16_t reservedSectorCount;           // Number of reserved sectors at the beginning of the partition
    uint8_t fatCount;                       // Number of FATs on the partition
    uint16_t rootDirectoryEntryCount;       // Number of root directory entries
    uint16_t totalSectors16;                // Total number of sectors
    uint8_t  mediaDescriptor;               // Media descriptor
    uint16_t sectorsPerFat16;               // Number of sectors per FAT (16-bit)
    uint16_t sectorsPerTrack;               // Number of sectors per track
    uint16_t headCount;                     // Number of heads
    uint32_t hiddenSectorCount;             // Number of hidden sectors
    uint32_t totalSectors32;                // Total sector count (32 bits)
    uint32_t sectorsPerFat32;               // Sectors per FAT (32 bits)
    uint16_t mirroringFlags;                // Presently active FAT. Defined by bits 0-3 if bit 7 is 1.
    uint16_t fileSystemVersion;             // FAT32 filesystem version.  Should be 0:0
    uint32_t firstClusterRootDirectory;     // Start cluster of the root directory (should be 2)
    uint16_t fileSystemInformation;         // File system information
    uint16_t backupBootSector;              // Backup boot sector address.
    uint8_t  reserved0[12];                 // Reserved space
    uint8_t  driveNumber;                   // Drive number
    uint8_t  reserved1;                     // Reserved space
    uint8_t  bootSignature;                 // Boot signature - 0x29
    uint8_t  volumeId[4];                   // Volume ID
    uint8_t  volumeLabel[11];               // Volume Label
    uint8_t  fileSystemType[8];             // File system type in ASCII.  Not used for determination
#if defined __XC32__ || defined __XC16__
} __attribute__ ((packed)) FILEIO_BIOS_PARAMETER_BLOCK_FAT32;
#else
} FILEIO_BIOS_PARAMETER_BLOCK_FAT32;
#endif


// A macro for the boot sector uint8_ts per sector value offset
#define BSI_BPS            11
// A macro for the boot sector sector per cluster value offset
#define BSI_SPC            13
// A macro for the boot sector reserved sector count value offset
#define BSI_RESRVSEC       14
// A macro for the boot sector FAT count value offset
#define BSI_FATCOUNT       16
// A macro for the boot sector root directory entry count value offset
#define BSI_ROOTDIRENTS    17
// A macro for the boot sector 16-bit total sector count value offset
#define BSI_TOTSEC16       19
// A macro for the boot sector sectors per FAT value offset
#define BSI_SPF            22
// A macro for the boot sector 32-bit total sector count value offset
#define BSI_TOTSEC32       32
// A macro for the boot sector boot signature offset
#define BSI_BOOTSIG        38
// A macro for the boot sector file system type string offset
#define BSI_FSTYPE         54
// A macro for the boot sector 32-bit sector per FAT value offset
#define  BSI_FATSZ32       36
// A macro for the boot sector start cluster of root directory value offset
#define  BSI_ROOTCLUS      44
//  A macro for the FAT32 boot sector boot signature offset
#define  BSI_FAT32_BOOTSIG 66
// A macro for the FAT32 boot sector file system type string offset
#define  BSI_FAT32_FSTYPE  82


// Structure of a partition table entry
typedef struct
{
    uint8_t bootDescriptor;                 // The boot descriptor (should be 0x00 in a non-bootable device)
    uint8_t chsFirstParitionSector[3];      // The cylinder-head-sector address of the first sector of the partition
    uint8_t fileSystemDescriptor;           // The file system descriptor
    uint8_t chsLastPartitionSector[3];      // The cylinder-head-sector address of the last sector of the partition
    uint32_t lbaFirstSector;                // The logical block address of the first sector of the partition
    uint32_t sectorCount;                   // The number of sectors in a partition
#if defined __XC32__ || defined __XC16__
} __attribute__ ((packed)) FILEIO_MBR_PARTITION_TABLE_ENTRY;
#else
} FILEIO_MBR_PARTITION_TABLE_ENTRY;
#endif

// Strucure of a device's master boot record
typedef struct
{
    uint8_t bootCode[446];                          // Boot code
    FILEIO_MBR_PARTITION_TABLE_ENTRY partition0;    // The first partition table entry
    FILEIO_MBR_PARTITION_TABLE_ENTRY partition1;    // The second partition table entry
    FILEIO_MBR_PARTITION_TABLE_ENTRY partition2;    // The third partition table entry
    FILEIO_MBR_PARTITION_TABLE_ENTRY partition3;    // The fourth partition table entry
    uint8_t signature0;                             // MBR signature code - equal to 0x55
    uint8_t signature1;                             // MBR signature code - equal to 0xAA
#if defined __XC32__ || defined __XC16__
}__attribute__((packed)) FILEIO_MASTER_BOOT_RECORD;
#else
} FILEIO_MASTER_BOOT_RECORD;
#endif

// Structure matching the configuration of a FAT boot sector
typedef struct
{
    // A union of different bios parameter blocks
    union
    {
        FILEIO_BIOS_PARAMETER_BLOCK_FAT32 fat32;
        FILEIO_BIOS_PARAMETER_BLOCK_FAT16 fat16;
        FILEIO_BIOS_PARAMETER_BLOCK_FAT12 fat12;
    } biosParameterBlock;
    uint8_t reserved[512-sizeof(FILEIO_BIOS_PARAMETER_BLOCK_FAT32)-2];      // Reserved space
    uint8_t signature0;                                                     // Boot sector signature code - equal to 0x55
    uint8_t signature1;                                                     // Boot sector signature code - equal to 0xAA
#if defined __XC32__ || defined __XC16__
    } __attribute__ ((packed)) FILEIO_BOOT_SECTOR;
#else
    } FILEIO_BOOT_SECTOR;
#endif




FILEIO_ERROR_TYPE FILEIO_LoadMBR (FILEIO_DRIVE * drive);
FILEIO_ERROR_TYPE FILEIO_LoadBootSector (FILEIO_DRIVE * drive);
uint32_t FILEIO_FullClusterNumberGet(FILEIO_DIRECTORY_ENTRY * entry);
uint32_t FILEIO_ClusterToSector(FILEIO_DRIVE * disk, uint32_t cluster);
FILEIO_DRIVE * FILEIO_CharToDrive (char c);
const char * FILEIO_CacheDirectory (FILEIO_DIRECTORY * dir, const char * path, bool createDirectories);
uint16_t FILEIO_FindNextDelimiter(const char * path);
FILEIO_RESULT FILEIO_DirectoryMakeSingle (FILEIO_DIRECTORY * dir, const char * path);
FILEIO_RESULT FILEIO_DirectoryChangeSingle (FILEIO_DIRECTORY * dir, const char * path);
int FILEIO_DirectoryRemoveSingle (FILEIO_DIRECTORY * directory, char * path);
void FILEIO_FormatShortFileName (const char * fileName, FILEIO_OBJECT * filePtr);
uint8_t FILEIO_FileNameTypeGet (const char * fileName, bool partialStringSearch);
bool FILEIO_ShortFileNameCompare (uint8_t * fileName1, uint8_t * fileName2, uint8_t mode);
uint32_t FILEIO_FATWrite (FILEIO_DRIVE *disk, uint32_t currentCluster, uint32_t value, uint8_t forceWrite);
uint32_t FILEIO_FATRead (FILEIO_DRIVE * disk, uint32_t currentCluster);
FILEIO_DIRECTORY_ENTRY * FILEIO_DirectoryEntryCache (FILEIO_DIRECTORY * directory, FILEIO_ERROR_TYPE * error, uint32_t * currentCluster, uint16_t * currentClusterOffset, uint16_t entryOffset);
bool FILEIO_FlushBuffer (FILEIO_DRIVE * disk, FILEIO_BUFFER_ID bufferId);
FILEIO_ERROR_TYPE FILEIO_EraseClusterChain (uint32_t cluster, FILEIO_DRIVE * disk);
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryCreate (FILEIO_OBJECT * filePtr, uint16_t * entryHandle, uint8_t attributes, bool allocateDataCluster);
FILEIO_ERROR_TYPE FILEIO_ClusterAllocate (FILEIO_DRIVE * drive, uint32_t * cluster, bool eraseCluster);
FILEIO_ERROR_TYPE FILEIO_EraseCluster (FILEIO_DRIVE * drive, uint32_t cluster);
uint32_t FILEIO_FindEmptyCluster (FILEIO_DRIVE * drive, uint32_t baseCluster);
uint32_t FILEIO_CreateFirstCluster (FILEIO_OBJECT * filePtr);
FILEIO_ERROR_TYPE FILEIO_FindShortFileName (FILEIO_DIRECTORY * directory, FILEIO_OBJECT * filePtr, uint8_t * fileName, uint32_t * currentCluster, uint16_t * currentClusterOffset, uint16_t entryOffset, uint16_t attributes, FILEIO_SEARCH_TYPE mode);
FILEIO_ERROR_TYPE FILEIO_EraseFile (FILEIO_OBJECT * filePtr, uint16_t * entryHandle, bool eraseData);
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryFindEmpty (FILEIO_OBJECT * filePtr, uint16_t * entryOffset);
FILEIO_ERROR_TYPE FILEIO_DirectoryEntryPopulate(FILEIO_OBJECT * filePtr, uint16_t * entryHandle, uint8_t attributes, uint32_t cluster);
FILEIO_ERROR_TYPE FILEIO_NextClusterGet (FILEIO_OBJECT * fo, uint32_t count);
int FILEIO_DotEntryWrite (FILEIO_DRIVE * drive, uint32_t dot, uint32_t dotdot, FILEIO_TIMESTAMP * timeStamp);
void FILEIO_ShortFileNameConvert (char * newFileName, char * oldFileName);
bool FILEIO_IsClusterAllocated(FILEIO_DIRECTORY * directory, FILEIO_OBJECT * filePtr);
int FILEIO_GetSingleBuffer (FILEIO_DRIVE * drive);
FILEIO_ERROR_TYPE FILEIO_ForceRecache (FILEIO_DRIVE * disk);

#endif
