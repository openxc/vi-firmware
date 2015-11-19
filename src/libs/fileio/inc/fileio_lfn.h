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


#ifndef  _FILEIO_DOT_H
#define  _FILEIO_DOT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system_config.h"
#include "system.h"


/*******************************************************************/
/*                     Structures and defines                     */
/*******************************************************************/

// Enumeration for general purpose return values
typedef enum
{
    FILEIO_RESULT_SUCCESS = 0,                  // File operation was a success
    FILEIO_RESULT_FAILURE = -1                  // File operation failed
} FILEIO_RESULT;

// Definition to indicate an invalid file handle
#define FILEIO_INVALID_HANDLE       NULL

#define FILEIO_FILE_NAME_LENGTH_8P3                 12          // Maximum file name length for 8.3 formatted files
#define FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX        11          // Maximum file name length for 8.3 formatted files with no radix

// Enumeration for formatting modes
typedef enum
{
    FILEIO_FORMAT_ERASE = 0,            // Erases the contents of the partition
    FILEIO_FORMAT_BOOT_SECTOR           // Creates a boot sector based on user-specified information and erases any existing information
} FILEIO_FORMAT_MODE;

// Enumeration for specific return codes
typedef enum
{
    FILEIO_ERROR_NONE = 0,                      // No error
    FILEIO_ERROR_ERASE_FAIL,                    // An erase failed
    FILEIO_ERROR_NOT_PRESENT,                   // No device was present
    FILEIO_ERROR_NOT_FORMATTED,                 // The disk is of an unsupported format
    FILEIO_ERROR_BAD_PARTITION,                 // The boot record is bad
    FILEIO_ERROR_UNSUPPORTED_FS,                // The file system type is unsupported
    FILEIO_ERROR_INIT_ERROR,                    // An initialization error has occured
    FILEIO_ERROR_UNINITIALIZED,                 // An operation was performed on an uninitialized device
    FILEIO_ERROR_BAD_SECTOR_READ,               // A bad read of a sector occured
    FILEIO_ERROR_WRITE,                         // Could not write to a sector
    FILEIO_ERROR_INVALID_CLUSTER,               // Invalid cluster value > maxcls
    FILEIO_ERROR_DRIVE_NOT_FOUND,               // The specified drive could not be found
    FILEIO_ERROR_FILE_NOT_FOUND,                // Could not find the file on the device
    FILEIO_ERROR_DIR_NOT_FOUND,                 // Could not find the directory
    FILEIO_ERROR_BAD_FILE,                      // File is corrupted
    FILEIO_ERROR_DONE,                          // No more files in this directory
    FILEIO_ERROR_COULD_NOT_GET_CLUSTER,         // Could not load/allocate next cluster in file
    FILEIO_ERROR_FILENAME_TOO_LONG,             // A specified file name is too long to use
    FILEIO_ERROR_FILENAME_EXISTS,               // A specified filename already exists on the device
    FILEIO_ERROR_INVALID_FILENAME,              // Invalid file name
    FILEIO_ERROR_DELETE_DIR,                    // The user tried to delete a directory with FILEIO_Remove
    FILEIO_ERROR_DELETE_FILE,                   // The user tried to delete a file with FILEIO_DirectoryRemove
    FILEIO_ERROR_DIR_FULL,                      // All root dir entry are taken
    FILEIO_ERROR_DRIVE_FULL,                     // All clusters in partition are taken
    FILEIO_ERROR_DIR_NOT_EMPTY,                 // This directory is not empty yet, remove files before deleting
    FILEIO_ERROR_UNSUPPORTED_SIZE,              // The disk is too big to format as FAT16
    FILEIO_ERROR_WRITE_PROTECTED,               // Card is write protected
    FILEIO_ERROR_FILE_UNOPENED,                 // File not opened for the write
    FILEIO_ERROR_SEEK_ERROR,                    // File location could not be changed successfully
    FILEIO_ERROR_BAD_CACHE_READ,                // Bad cache read
    FILEIO_ERROR_FAT32_UNSUPPORTED,             // FAT 32 - card not supported
    FILEIO_ERROR_READ_ONLY,                     // The file is read-only
    FILEIO_ERROR_WRITE_ONLY,                    // The file is write-only
    FILEIO_ERROR_INVALID_ARGUMENT,              // Invalid argument
    FILEIO_ERROR_TOO_MANY_FILES_OPEN,           // Too many files are already open
    FILEIO_ERROR_TOO_MANY_DRIVES_OPEN,          // Too many drives are already open
    FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE,       // Unsupported sector size
    FILEIO_ERROR_NO_LONG_FILE_NAME,             // Long file name was not found
    FILEIO_ERROR_EOF                            // End of file reached
} FILEIO_ERROR_TYPE;

// Enumeration defining standard attributes used by FAT file systems
typedef enum
{
    FILEIO_ATTRIBUTE_READ_ONLY = 0x01,          // Read-only attribute.  A file with this attribute should not be written to.
    FILEIO_ATTRIBUTE_HIDDEN = 0x02,             // Hidden attribute.  A file with this attribute may be hidden from the user.
    FILEIO_ATTRIBUTE_SYSTEM = 0x04,             // System attribute.  A file with this attribute is used by the operating system and should not be modified.
    FILEIO_ATTRIBUTE_VOLUME = 0x08,             // Volume attribute.  If the first file in the root directory of a volume has this attribute, the entry name is the volume name.
    FILEIO_ATTRIBUTE_LONG_NAME = 0x0F,          // A file entry with this attribute mask is used to store part of the file's Long File Name.
    FILEIO_ATTRIBUTE_DIRECTORY = 0x10,          // A file entry with this attribute points to a directory.
    FILEIO_ATTRIBUTE_ARCHIVE = 0x20,            // Archive attribute.  A file with this attribute should be archived.
    FILEIO_ATTRIBUTE_MASK = 0x3F                // Mask for all attributes.
} FILEIO_ATTRIBUTES;

// Enumeration defining base locations for seeking
typedef enum
{
    FILEIO_SEEK_SET = 0,                // Change the position in the file to an offset relative to the beginning of the file.
    FILEIO_SEEK_CUR,                    // Change the position in the file to an offset relative to the current location in the file.
    FILEIO_SEEK_END                     // Change the position in the file to an offset relative to the end of the file.
} FILEIO_SEEK_BASE;

// Enumeration for file access modes
typedef enum
{
    FILEIO_OPEN_READ = 0x01,            // Open the file for reading.
    FILEIO_OPEN_WRITE = 0x02,           // Open the file for writing.
    FILEIO_OPEN_CREATE = 0x04,          // Create the file if it doesn't exist.
    FILEIO_OPEN_TRUNCATE = 0x08,        // Truncate the file to 0-length.
    FILEIO_OPEN_APPEND = 0x10           // Set the current read/write location in the file to the end of the file.
} FILEIO_OPEN_ACCESS_MODES;

// Enumeration of macros defining possible file system types supported by a device
typedef enum
{
    FILEIO_FILE_SYSTEM_TYPE_NONE = 0,       // No file system
    FILEIO_FILE_SYSTEM_TYPE_FAT12,          // The device is formatted with FAT12
    FILEIO_FILE_SYSTEM_TYPE_FAT16,          // The device is formatted with FAT16
    FILEIO_FILE_SYSTEM_TYPE_FAT32           // The device is formatted with FAT32
} FILEIO_FILE_SYSTEM_TYPE;

// Summary: Contains file information and is used to indicate which file to access.
// Description: The FILEIO_OBJECT structure is used to hold file information for an open file as it's being modified or accessed.  A pointer to
//              an open file's FILEIO_OBJECT structure will be passed to any library function that will modify that file.
typedef struct
{
    uint32_t        baseClusterDir;     // The base cluster of the file's directory
    uint32_t        currentClusterDir;  // The current cluster of the file's directory
    uint32_t        firstCluster;       // The first cluster of the file
    uint32_t        currentCluster;     // The current cluster of the file
    uint32_t        size;               // The size of the file
    uint32_t        absoluteOffset;     // The absolute offset in the file
    void *          disk;               // Pointer to a device structure
    uint16_t *      lfnPtr;             // Pointer to a LFN buffer
    uint16_t        lfnLen;             // Length of the long file name
    uint16_t        currentSector;      // The current sector in the current cluster of the file
    uint16_t        currentOffset;      // The position in the current sector
    uint16_t        entry;              // The position of the file's directory entry in its directory
    uint16_t        attributes;         // The file's attributes
    uint16_t        time;               // The file's last update time
    uint16_t        date;               // The file's last update date
    uint8_t         timeMs;             // The file's last update time (ms portion)
    char            name[FILEIO_FILE_NAME_LENGTH_8P3_NO_RADIX];     // The short name of the file
    struct
    {
        unsigned    writeEnabled :1;    // Indicates a file was opened in a mode that allows writes
        unsigned    readEnabled :1;     // Indicates a file was opened in a mode that allows reads

    } flags;
} FILEIO_OBJECT;

// Possible results of the FSGetDiskProperties() function.
typedef enum
{
    FILEIO_GET_PROPERTIES_NO_ERRORS = 0,
    FILEIO_GET_PROPERTIES_CACHE_ERROR,
    FILEIO_GET_PROPERTIES_DRIVE_NOT_MOUNTED,
    FILEIO_GET_PROPERTIES_CLUSTER_FAILURE,
    FILEIO_GET_PROPERTIES_STILL_WORKING = 0xFF
} FILEIO_DRIVE_ERRORS;

// Enumeration to define media error types
typedef enum
{
    MEDIA_NO_ERROR,                     // No errors
    MEDIA_DEVICE_NOT_PRESENT,           // The requested device is not present
    MEDIA_CANNOT_INITIALIZE             // Cannot initialize media
} FILEIO_MEDIA_ERRORS;

// Media information flags.  The driver's MediaInitialize function will return a pointer to one of these structures.
typedef struct
{
    FILEIO_MEDIA_ERRORS errorCode;              // The status of the intialization FILEIO_MEDIA_ERRORS
    // Flags
    union
    {
        uint8_t    value;
        struct
        {
            uint8_t    sectorSize  : 1;         // The sector size parameter is valid.
            uint8_t    maxLUN      : 1;         // The max LUN parameter is valid.
        }   bits;
    } validityFlags;

    uint16_t    sectorSize;                     // The sector size of the target device.
    uint8_t    maxLUN;                          // The maximum Logical Unit Number of the device.
} FILEIO_MEDIA_INFORMATION;

/***************************************************************************
    Function:
        void (*FILEIO_DRIVER_IOInitialize)(void * mediaConfig);

    Summary:
        Function pointer prototype for a driver function to initialize
        I/O pins and modules for a driver.

    Description:
        Function pointer prototype for a driver function to initialize
        I/O pins and modules for a driver.

    Precondition:
        None

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure

    Returns:
        None
***************************************************************************/
typedef void (*FILEIO_DRIVER_IOInitialize)(void * mediaConfig);

/***************************************************************************
    Function:
        bool (*FILEIO_DRIVER_MediaDetect)(void * mediaConfig);

    Summary:
        Function pointer prototype for a driver function to detect if
        a media device is attached/available.

    Description:
        Function pointer prototype for a driver function to detect if
        a media device is attached/available.

    Precondition:
        None

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure

    Returns:
        If media attached: true
        If media not atached: false
***************************************************************************/
typedef bool (*FILEIO_DRIVER_MediaDetect)(void * mediaConfig);

/***************************************************************************
    Function:
        FILEIO_MEDIA_INFORMATION * (*FILEIO_DRIVER_MediaInitialize)(void * mediaConfig);

    Summary:
        Function pointer prototype for a driver function to perform media-
        specific initialization tasks.

    Description:
        Function pointer prototype for a driver function to perform media-
        specific initialization tasks.

    Precondition:
        FILEIO_DRIVE_IOInitialize will be called first.

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure

    Returns:
        FILEIO_MEDIA_INFORMATION * - Pointer to a media initialization structure
            that has been loaded with initialization values.
***************************************************************************/
typedef FILEIO_MEDIA_INFORMATION * (*FILEIO_DRIVER_MediaInitialize)(void * mediaConfig);

/***************************************************************************
    Function:
        bool (*FILEIO_DRIVER_MediaDeinitialize)(void * mediaConfig);

    Summary:
        Function pointer prototype for a driver function to deinitialize
        a media device.

    Description:
        Function pointer prototype for a driver function to deinitialize
        a media device.

    Precondition:
        None

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure

    Returns:
        If Success: true
        If Failure: false
***************************************************************************/
typedef bool (*FILEIO_DRIVER_MediaDeinitialize)(void * mediaConfig);

/***************************************************************************
    Function:
        bool (*FILEIO_DRIVER_SectorRead)(void * mediaConfig,
            uint32_t sector_addr, uint8_t * buffer);

    Summary:
        Function pointer prototype for a driver function to read a sector
        of data from the device.

    Description:
        Function pointer prototype for a driver function to read a sector
        of data from the device.

    Precondition:
        The device will be initialized.

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure
        sectorAddress - The address of the sector to read.  This address
            format depends on the media.
        buffer - A buffer to store the copied data sector.

    Returns:
        If Success: true
        If Failure: false
***************************************************************************/
typedef bool (*FILEIO_DRIVER_SectorRead)(void * mediaConfig, uint32_t sector_addr, uint8_t* buffer);

/***************************************************************************
    Function:
        bool (*FILEIO_DRIVER_SectorWrite)(void * mediaConfig,
            uint32_t sectorAddress, uint8_t * buffer, bool allowWriteToZero);

    Summary:
        Function pointer prototype for a driver function to write a sector
        of data to the device.

    Description:
        Function pointer prototype for a driver function to write a sector
        of data to the device.

    Precondition:
        The device will be initialized.

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure
        sectorAddress - The address of the sector to write. This address
            format depends on the media.
        buffer - A buffer containing the data to write.
		allowWriteToZero - Check to prevent writing to the master boot 
		    record.  This will always be false on calls that write to files, 
			which will prevent a device from accidentally overwriting its 
			own MBR if its root or FAT are corrupted.  This should only 
			be true if the user specifically tries to construct a new MBR.

    Returns:
        If Success: true
        If Failure: false
***************************************************************************/
typedef uint8_t (*FILEIO_DRIVER_SectorWrite)(void * mediaConfig, uint32_t sector_addr, uint8_t* buffer, bool allowWriteToZero);

/***************************************************************************
    Function:
        bool (*FILEIO_DRIVER_WriteProtectStateGet)(void * mediaConfig);

    Summary:
        Function pointer prototype for a driver function to determine if
        the device is write-protected.

    Description:
        Function pointer prototype for a driver function to determine if
        the device is write-protected.

    Precondition:
        None

    Parameters:
        mediaConfig - Pointer to a driver-defined config structure

    Returns:
        If write-protected: true
        If not write-protected: false
***************************************************************************/
typedef bool (*FILEIO_DRIVER_WriteProtectStateGet)(void * mediaConfig);


// Function pointer table that describes a drive being configured by the user
typedef struct
{
    FILEIO_DRIVER_IOInitialize funcIOInit;                          // I/O Initialization function
    FILEIO_DRIVER_MediaDetect funcMediaDetect;                      // Media Detection function
    FILEIO_DRIVER_MediaInitialize funcMediaInit;                    // Media Initialization function
    FILEIO_DRIVER_MediaDeinitialize funcMediaDeinit;                // Media Deinitialization function.
    FILEIO_DRIVER_SectorRead funcSectorRead;                        // Function to read a sector of the media.
    FILEIO_DRIVER_SectorWrite funcSectorWrite;                      // Function to write a sector of the media.
    FILEIO_DRIVER_WriteProtectStateGet funcWriteProtectGet;         // Function to determine if the media is write-protected.
} FILEIO_DRIVE_CONFIG;

// Structure that contains the disk search information, intermediate values, and results
typedef struct
{
    char    disk;           /* pointer to the disk we are searching */
    bool    new_request;    /* is this a new request or a continued request */
    FILEIO_DRIVE_ERRORS properties_status;  /* status of the last call of the function */

    struct
    {
        uint8_t disk_format;           /* disk format: FAT12, FAT16, FAT32 */
        uint16_t sector_size;           /* sector size of the drive */
        uint8_t sectors_per_cluster;   /* number of sectors per cluster */
        uint32_t total_clusters;       /* the number of total clusters on the drive */
        uint32_t free_clusters;        /* the number of free (unused) clusters on drive */
    } results;                      /* the results of the current search */

    struct
    {
        uint32_t   c;     
        uint32_t   curcls;
        uint32_t   EndClusterLimit;
        uint32_t   ClusterFailValue;
    } private;      /* intermediate values used to continue searches.  This
                         member should be used only by the FSGetDiskProperties()
                         function */

} FILEIO_DRIVE_PROPERTIES;

// Structure to describe a FAT file system date
typedef union
{
    struct
    {
        uint16_t day : 5;           // Day (1-31)
        uint16_t month : 4;         // Month (1-12)
        uint16_t year : 7;          // Year (number of years since 1980)
    } bitfield;
    uint16_t value;
} FILEIO_DATE;

// Function to describe the FAT file system time.
typedef union
{
    struct
    {
        uint16_t secondsDiv2 : 5;   // (Seconds / 2) ( 1-30)
        uint16_t minutes : 6;       // Minutes ( 1-60)
        uint16_t hours : 5;         // Hours (1-24)
    } bitfield;
    uint16_t value;
} FILEIO_TIME;

// Structure to describe the time fields of a file
typedef struct  
{
    FILEIO_DATE date;               // The create or write date of the file/directory.
    FILEIO_TIME time;               // The create of write time of the file/directory.
    uint8_t timeMs;                 // The millisecond portion of the time.
} FILEIO_TIMESTAMP;

// Search structure
typedef struct
{
    // Return values

    uint8_t shortFileName[13];          // The name of the file that has been found (NULL-terminated).
    uint8_t attributes;                 // The attributes of the file that has been found.
    uint32_t fileSize;                  // The size of the file that has been found (bytes).
    FILEIO_TIMESTAMP timeStamp;        // The create or write time of the file that has been found.

    // Private Parameters
    uint32_t baseDirCluster;
    uint32_t currentDirCluster;
    uint16_t currentClusterOffset;
    uint16_t currentEntryOffset;
    uint16_t pathOffset;
    uint16_t driveId;
} FILEIO_SEARCH_RECORD;

/***************************************************************************
* Prototypes                                                               *
***************************************************************************/

/***************************************************************************
  Function:
    int FILEIO_Initialize (void)

    Summary:
        Initialized the FILEIO library.

    Description:
        Initializes the structures used by the FILEIO library.

    Precondition:
        None.

    Parameters:
        void

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                 
***************************************************************************/
int FILEIO_Initialize (void);

/***************************************************************************
  Function:
    int FILEIO_Reinitialize (void)

    Summary:
        Reinitialized the FILEIO library.

    Description:
        Reinitialized the structures used by the FILEIO library.

    Precondition:
        FILEIO_Initialize must have been called.

    Parameters:
        void

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                   
***************************************************************************/
int FILEIO_Reinitialize (void);

/***************************************************************************
  Function:
    typedef void (*FILEIO_TimestampGet)(FILEIO_TIMESTAMP *)

    Summary:
        Describes the user-implemented function to provide the timestamp.

    Description:
        Files in a FAT files system use time values to track create time,
        access time, and last-modified time.  In the FILEIO library, the 
        user must implement a function that the library can call to 
        obtain the current time.  That function will have this format.

    Precondition:
        N/A.

    Parameters:
        FILEIO_TIMESTAMP * - Pointer to a timestamp structure that 
            must be populated by the user's function.

    Returns:
        void
***************************************************************************/
typedef void (*FILEIO_TimestampGet)(FILEIO_TIMESTAMP *);

/***************************************************************************
  Function:
    void FILEIO_RegisterTimestampGet (FILEIO_TimestampGet timestampFunction)

    Summary:
        Registers a FILEIO_TimestampGet function with the library.

    Description:
        The user must call this function to specify which user-implemented 
        function will be called by the library to generate timestamps.

    Precondition:
        FILEIO_Initialize must have been called.

    Parameters:
        timestampFunction - A pointer to the user-implemented function
            that will provide timestamps to the library.

    Returns:
        void
***************************************************************************/
void FILEIO_RegisterTimestampGet (FILEIO_TimestampGet timestampFunction);

/***************************************************************************
  Function:
    bool FILEIO_MediaDetect (const FILEIO_DRIVE_CONFIG * driveConfig,
        void * mediaParameters)

    Summary:
        Determines if the given media is accessible.

    Description:
        This function determines if a specified media device is available
        for further access.

    Precondition:
        FILEIO_Initialize must have been called.  The driveConfig struct
        must have been initialized with the media-specific parameters and
        the FILEIO_DRIVER_MediaDetect function.

    Parameters:
        driveConfig - Constant structure containing function pointers that
            the library will use to access the drive.
        mediaParameters - Pointer to the media-specific parameter structure

    Returns:
      * If media is available : true
      * If media is not available : false                                  
***************************************************************************/
bool FILEIO_MediaDetect (const FILEIO_DRIVE_CONFIG * driveConfig, void * mediaParameters);

/*****************************************************************************
  Function:
      FILEIO_ERROR_TYPE FILEIO_DriveMount (uint16_t driveId,
          const FILEIO_DRIVE_CONFIG * driveConfig,
          void * mediaParameters);
    
  Summary:
    Initializes a drive and loads its configuration information.
  Description:
    This function will initialize a drive and load the required information
    from it.
  Conditions:
    FILEIO_Initialize must have been called.
  Input:
    driveId -          A Unicode character that will be used to identify the
                       drive.
    driveConfig -      Constant structure containing function pointers that
                       the library will use to access the drive.
    mediaParameters -  Constant structure containing media\-specific values
                       that describe which instance of the media to use for
                       this operation.
  Return:
      * FILEIO_ERROR_NONE - Drive was mounted successfully
      * FILEIO_ERROR_TOO_MANY_DRIVES_OPEN - You have already mounted
        the maximum number of drives. Change FILEIO_CONFIG_MAX_DRIVES in
        fileio_config.h to increase this.
      * FILEIO_ERROR_WRITE - The library was not able to write cached
        data in the buffer to the device (can occur when using multiple drives
        and single buffer mode)
      * FILEIO_ERROR_INIT_ERROR - The driver's Media Initialize
        \function indicated that the media could not be initialized.
      * FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE - The media's sector size
        exceeds the maximum sector size specified in fileio_config.h
        (FILEIO_CONFIG_MEDIA_SECTOR_SIZE macro)
      * FILEIO_ERROR_BAD_SECTOR_READ - The stack could not read the
        boot sector of Master Boot Record from the media.
      * FILEIO_ERROR_BAD_PARTITION - The boot signature in the MBR is
        bad on your media device.
      * FILEIO_ERROR_UNSUPPORTED_FS - The partition is formatted with
        an unsupported file system.
      * FILEIO_ERROR_NOT_FORMATTED - One of the parameters in the boot
        sector is bad in the partition being mounted.                         
  *****************************************************************************/
FILEIO_ERROR_TYPE FILEIO_DriveMount (uint16_t driveId, const FILEIO_DRIVE_CONFIG * driveConfig, void * mediaParameters);

/***************************************************************************
    Function:
        int FILEIO_Format (FILEIO_DRIVE_CONFIG * config,
            void * mediaParameters, char mode,
            uint32_t serialNumber, char * volumeID)

    Summary:
        Formats a drive.

    Description:
        Formats a drive.

    Precondition:
        FILEIO_Initialize must have been called.

    Parameters:
        config - Drive configuration pointer
        mode - FILEIO_FORMAT_MODE specifier
        serialNumber - Serial number to write to the drive
        volumeId - Name of the drive.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                          
***************************************************************************/
int FILEIO_Format (FILEIO_DRIVE_CONFIG * config, void * mediaParameters, FILEIO_FORMAT_MODE mode, uint32_t serialNumber, char * volumeId);

/***********************************************************************
  Function:
      int FILEIO_DriveUnmount (const uint16_t driveID)
    
  Summary:
    Unmounts a drive.
  Description:
    Unmounts a drive from the file system and writes any pending data to
    the drive.
  Conditions:
    FILEIO_DriveMount must have been called.
  Input:
    driveId -  The character representation of the mounted drive.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                               
  ***********************************************************************/
int FILEIO_DriveUnmount (const uint16_t driveId);

/******************************************************************************
  Function:
      int FILEIO_Remove (const char * pathName)
    
  Summary:
    Deletes a file.
  Description:
    Deletes the file specified by pathName.
  Conditions:
    The file's drive must be mounted and the file should exist.
  Input:
    pathName -  The path/name of the file.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet. Note
        that if the path cannot be resolved, the error will be returned for the
        current working directory.
        * FILEIO_ERROR_INVALID_ARGUMENT - The path could not be
          resolved.
        * FILEIO_ERROR_WRITE_PROTECTED - The device is write-protected.
        * FILEIO_ERROR_INVALID_FILENAME - The file name is invalid.
        * FILEIO_ERROR_DELETE_DIR - The file being deleted is actually
          a directory (use FILEIO_DirectoryRemove)
        * FILEIO_ERROR_ERASE_FAIL - The erase operation failed.
        * FILEIO_ERROR_FILE_NOT_FOUND - The file entries for this file
          are invalid or have already been erased.
        * FILEIO_ERROR_WRITE - The updated file data and entry could
          not be written to the device.
        * FILEIO_ERROR_DONE - The directory entry could not be found.
        * FILEIO_ERROR_BAD_SECTOR_READ - The directory entry could not
          be cached.                                                           
  ******************************************************************************/
int FILEIO_Remove (const uint16_t * pathName);

/*******************************************************************************
  Function:
      int FILEIO_Rename (const uint16_t * oldPathname,
          const uint16_t * newFilename)
    
  Summary:
    Renames a file.
  Description:
    Renames a file specifed by oldPathname to the name specified by
    newFilename.
  Conditions:
    The file's drive must be mounted and the file/path specified by
    oldPathname must exist.
  Input:
    oldPathName -  The path/name of the file to rename.
    newFileName -  The new name of the file.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet Note
        that if the path cannot be resolved, the error will be returned for the
        current working directory.
        * FILEIO_ERROR_INVALID_ARGUMENT - The path could not be
          resolved.
        * FILEIO_ERROR_WRITE_PROTECTED - The device is write-protected.
        * FILEIO_ERROR_INVALID_FILENAME - One of the file names is
          invalid.
        * FILEIO_ERROR_FILENAME_EXISTS - The new file name already
          exists on this device.
        * FILEIO_ERROR_FILE_NOT_FOUND - The file could not be found.
        * FILEIO_ERROR_WRITE - The updated file data and entry could
          not be written to the device.
        * FILEIO_ERROR_DONE - The directory entry could not be found or
          the library could not find a sufficient number of empty entries in the
          dir to store the new file name.
        * FILEIO_ERROR_BAD_SECTOR_READ - The directory entry could not
          be cached.
        * FILEIO_ERROR_ERASE_FAIL - The file's entries could not be
          erased (applies when renaming a long file name)
        * FILEIO_ERROR_DIR_FULL - New file entries could not be
          created.
        * FILEIO_ERROR_BAD_CACHE_READ - The lfn entries could not be
          cached.                                                               
  *******************************************************************************/
int FILEIO_Rename (const uint16_t * oldPathName, const uint16_t * newFileName);

/************************************************************
  Function:
        int FILEIO_DirectoryMake (const uint16_t * path)
    
  Summary:
    Creates the directory/directories specified by 'path.'
	
  Description:
    Creates the directory/directories specified by 'path.'
	
  Conditions:
    The specified drive must be mounted.
	
  Input:
    path -  Path string containing all directories to create.
	
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                    
  ************************************************************/
int FILEIO_DirectoryMake (const uint16_t * path);

/*************************************************************************
  Function:
      int FILEIO_DirectoryChange (const uint16_t * path)
    
  Summary:
    Changes the current working directory.
	
  Description:
    Changes the current working directory to the directory specified by
    'path.'
	
  Conditions:
    The specified drive must be mounted and the directory being changed to
    should exist.
	
  Input:
    path -  The path of the directory to change to.
	
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                                 
  *************************************************************************/
int FILEIO_DirectoryChange (const uint16_t * path);

/******************************************************************************
  Function:
      uint16_t FILEIO_DirectoryGetCurrent (uint16_t * buffer, uint16_t size)
    
  Summary:
    Gets the name of the current working directory.
  Description:
    Gets the name of the current working directory and stores it in
    'buffer.' The directory name will be null-terminated. If the buffer
    size is insufficient to contain the whole path name, as much as
    possible will be copied and null-terminated.
  Conditions:
    A drive must be mounted.
  Input:
    buffer -  The buffer to contain the current working directory name.
    size -    Size of the buffer (16-bit words).
  Return:
      * uint16_t - The number of characters in the current working
        directory name. May exceed the size of the buffer. In this case, the
        name will be truncated to 'size' characters, but the full length of the
        path name will be returned.
      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_INVALID_ARGUMENT - The arguments for the buffer
          or its size were invalid.
        * FILEIO_ERROR_DIR_NOT_FOUND - One of the directories in your
          current working directory could not be found in its parent directory.
  ******************************************************************************/
uint16_t FILEIO_DirectoryGetCurrent (uint16_t * buffer, uint16_t size);

/************************************************************************
  Function:
        int FILEIO_DirectoryRemove (const uint16_t * pathName)
    
  Summary:
    Deletes a directory.
  Description:
    Deletes a directory. The specified directory must be empty.
  Conditions:
    The directory's drive must be mounted and the directory should exist.
  Input:
    pathName -  The path/name of the directory to delete.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE                                
  ************************************************************************/
int FILEIO_DirectoryRemove (const uint16_t * pathName);

/***************************************************************************
  Function:
    FILEIO_ERROR_TYPE FILEIO_ErrorGet (uint16_t driveId)

    Summary:
        Gets the last error condition of a drive.

    Description:
        Gets the last error condition of the specified drive.

    Precondition:
        The drive must have been mounted.

    Parameters:
        driveId - The character representation of the drive.

    Returns:
        FILEIO_ERROR_TYPE - The last error that occurred on the drive.
***************************************************************************/
FILEIO_ERROR_TYPE FILEIO_ErrorGet (uint16_t driveId);

/***************************************************************************
  Function:
    void FILEIO_ErrorClear (uint16_t driveId)

    Summary:
        Clears the last error on a drive.

    Description:
        Clears the last error of the specified drive.

    Precondition:
        The drive must have been mounted.

    Parameters:
        driveId - The character representation of the drive.

    Returns:
        void
***************************************************************************/
void FILEIO_ErrorClear (uint16_t driveId);

/***************************************************************************************
  Function:
      int FILEIO_Open (FILEIO_OBJECT * filePtr, const uint16_t * pathName, uint16_t mode)
    
  Summary:
    Opens a file for access.
  Description:
    Opens a file for access using a combination of modes specified by the
    user.
  Conditions:
    The drive containing the file must be mounted.
  Input:
    filePtr -   Pointer to the file object to initialize
    pathName -  The path/name of the file to open.
    mode -      The mode in which the file should be opened. Specified by
                inclusive or'ing parameters from FILEIO_OPEN_ACCESS_MODES.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet Note
        that if the path cannot be resolved, the error will be returned for the
        current working directory.
        * FILEIO_ERROR_INVALID_ARGUMENT - The path could not be
          resolved.
        * FILEIO_ERROR_WRITE_PROTECTED - The device is write protected
          or this function was called in a write/create mode when writes are
          disabled in configuration.
        * FILEIO_ERROR_INVALID_FILENAME - The file name is invalid.
        * FILEIO_ERROR_ERASE_FAIL - There was an error when trying to
          truncate the file.
        * FILEIO_ERROR_WRITE - Cached file data could not be written to
          the device.
        * FILEIO_ERROR_DONE - The directory entry could not be found.
        * FILEIO_ERROR_BAD_SECTOR_READ - The directory entry could not
          be cached.
        * FILEIO_ERROR_DRIVE_FULL - There are no more clusters
          available on this device that can be allocated to the file.
        * FILEIO_ERROR_FILENAME_EXISTS - All of the possible alias
          values for this file are in use.
        * FILEIO_ERROR_BAD_CACHE_READ - There was an error caching LFN
          entries.
        * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file
          is invalid (can occur in APPEND mode).
        * FILEIO_ERROR_COULD_NOT_GET_CLUSTER - There was an error
          finding the cluster that contained the specified offset (can occur in
          APPEND mode).                                                   
  ***************************************************************************************/
int FILEIO_Open (FILEIO_OBJECT * filePtr, const uint16_t * pathName, uint16_t mode);

/***************************************************************************
  Function:
    int FILEIO_Close (FILEIO_OBJECT * handle)

    Summary:
        Closes a file.

    Description:
        Closes a file.  This will save the unwritten data to the file and 
        make the memory used to allocate a file available to open other 
        files.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        handle - The handle of the file to close.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_WRITE - Data could not be written to the device.
        * FILEIO_ERROR_BAD_CACHE_READ - The file's directory entry
          could not be cached.                                             
***************************************************************************/
int FILEIO_Close (FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    int FILEIO_Flush (FILEIO_OBJECT * handle)

    Summary:
        Saves unwritten file data to the device without closing the file.

    Description:
        Saves unwritten file data to the device without closing the file. 
        This function is useful if the user needs to continue writing to 
        a file but also wants to ensure that data isn't lost in the event 
        of a reset or power loss condition.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.        

    Parameters:
        handle - The handle of the file to flush.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE

      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_WRITE - Data could not be written to the device.
        * FILEIO_ERROR_BAD_CACHE_READ - The file's directory entry
          could not be cached.                                             
***************************************************************************/
int FILEIO_Flush (FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    int FILEIO_GetChar (FILEIO_OBJECT * handle)

    Summary:
        Reads a character from a file.

    Description:
        Reads a character from a file.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        handle - The handle of the file.

    Returns:
      * If Success: The character that was read (cast to an int).
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_WRITE_ONLY - The file is not opened in read
          mode.
        * FILEIO_ERROR_BAD_SECTOR_READ - There was an error reading the
          FAT to determine the next cluster in the file, or an error reading the
          file data.
        * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file
          is invalid.
        * FILEIO_ERROR_EOF - There is no next cluster in the file (EOF)
        * FILEIO_ERROR_WRITE - Cached data could not be written to the
          device.                                                               
  *******************************************************************************/
int FILEIO_GetChar (FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    int FILEIO_PutChar (char c, FILEIO_OBJECT * handle)

    Summary:
        Writes a character to a file.

    Description:
        Writes a character to a file.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        c - The character to write.
        handle - The handle of the file.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_READ_ONLY - The file was not opened in write
          mode.
        * FILEIO_ERROR_WRITE_PROTECTED - The media is write-protected.
        * FILEIO_ERROR_BAD_SECTOR_READ - There was an error reading the
          FAT to determine the next cluster in the file, or an error reading the
          file data.
        * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file
          is invalid.
        * FILEIO_ERROR_WRITE - Cached data could not be written to the
          device.
        * FILEIO_ERROR_BAD_SECTOR_READ - File data could not be cached.
        * FILEIO_ERROR_DRIVE_FULL - There are no more clusters on the
          media that can be allocated to the file.                              
  *******************************************************************************/
int FILEIO_PutChar (char c, FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    size_t FILEIO_Read (void * buffer, size_t size, size_t count,
        FILEIO_OBJECT * handle)

    Summary:
        Reads data from a file.

    Description:
        Reads data from a file and stores it in 'buffer.'

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        buffer - The buffer that the data will be written to.
        size - The size of data objects to read, in bytes
        count - The number of data objects to read
        handle - The handle of the file.

    Returns:
    The number of data objects that were read. This value will match
    'count' if the read was successful, or be less than count if it was
    not.
    
    Sets error code which can be retrieved with FILEIO_ErrorGet:
      * FILEIO_ERROR_WRITE_ONLY - The file is not opened in read mode.
      * FILEIO_ERROR_BAD_SECTOR_READ - There was an error reading the
        FAT to determine the next cluster in the file, or an error reading the
        \file data.
      * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file is
        invalid.
      * FILEIO_ERROR_EOF - There is no next cluster in the file (EOF)
      * FILEIO_ERROR_WRITE - Cached data could not be written to the
        device.                                                               
  *****************************************************************************/
size_t FILEIO_Read (void * buffer, size_t size, size_t count, FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    size_t FILEIO_Write (void * buffer, size_t size, size_t count,
        FILEIO_OBJECT * handle)

    Summary:
        Writes data to a file.

    Description:
        Writes data from 'buffer' to a file.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        buffer - The buffer that contains the data to write.
        size - The size of data objects to write, in bytes
        count - The number of data objects to write
        handle - The handle of the file.

    Returns:
    The number of data objects that were written. This value will match
    'count' if the write was successful, or be less than count if it was
    not.
    
    Sets error code which can be retrieved with FILEIO_ErrorGet:
      * FILEIO_ERROR_READ_ONLY - The file was not opened in write mode.
      * FILEIO_ERROR_WRITE_PROTECTED - The media is write-protected.
      * FILEIO_ERROR_BAD_SECTOR_READ - There was an error reading the
        FAT to determine the next cluster in the file, or an error reading the
        file data.
      * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file is
        invalid.
      * FILEIO_ERROR_WRITE - Cached data could not be written to the
        device.
      * FILEIO_ERROR_BAD_SECTOR_READ - File data could not be cached.
      * FILEIO_ERROR_DRIVE_FULL - There are no more clusters on the
        media that can be allocated to the file.                              
  *****************************************************************************/
size_t FILEIO_Write (const void * buffer, size_t size, size_t count, FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    int FILEIO_Seek (FILEIO_OBJECT * handle, int32_t offset, int base)

    Summary:
        Changes the current read/write position in the file.

    Description:
        Changes the current read/write position in the file.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        handle - The handle of the file.
        offset - The offset of the new read/write position (in bytes) from 
            the base location.  The offset will be added to FILEIO_SEEK_SET 
            or FILEIO_SEEK_CUR, or subtracted from FILEIO_SEEK_END.
        base - The base location.  Is of the FILEIO_SEEK_BASE type.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet
        * FILEIO_ERROR_WRITE - Cached data could not be written to the
          device.
        * FILEIO_ERROR_INVALID_ARGUMENT - The specified location
          exceeds the file's size.
        * FILEIO_ERROR_BAD_SECTOR_READ - There was an error reading the
          FAT to determine the next cluster in the file, or an error reading the
          file data.
        * FILEIO_ERROR_INVALID_CLUSTER - The next cluster in the file
          is invalid.
        * FILEIO_ERROR_DRIVE_FULL - There are no more clusters on the
          media that can be allocated to the file. Clusters will be allocated to
          the file if the file is opened in a write mode and the user seeks to
          the end of a file that ends on a cluster boundary.
        * FILEIO_ERROR_COULD_NOT_GET_CLUSTER - There was an error
          finding the cluster that contained the specified offset.              
  *******************************************************************************/
int FILEIO_Seek (FILEIO_OBJECT * handle, int32_t offset, int base);

/***************************************************************************
  Function:
    bool FILEIO_Eof (FILEIO_OBJECT * handle)

    Summary:
        Determines if the file's current read/write position is at the end 
        of the file.

    Description:
        Determines if the file's current read/write position is at the end 
        of the file.        

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        handle - The handle of the file.

    Returns:
      * If EOF: true
      * If Not EOF: false                                                 
  *************************************************************************/
bool FILEIO_Eof (FILEIO_OBJECT * handle);

/***************************************************************************
  Function:
    long FILEIO_Tell (FILEIO_OBJECT * handle)

    Summary:
        Returns the current read/write position in the file.

    Description:
        Returns the current read/write position in the file.

    Precondition:
        The drive containing the file must be mounted and the file handle 
        must represent a valid, opened file.

    Parameters:
        handle - THe handle of the file.

    Returns:
        long - Offset of the current read/write position from the beginning 
            of the file, in bytes.
***************************************************************************/
long FILEIO_Tell (FILEIO_OBJECT * handle);

/******************************************************************************
  Function:
      int FILEIO_Find (const char * fileName, unsigned int attr,
          FILEIO_SEARCH_RECORD * record, bool newSearch)
    
  Summary:
    Searches for a file in the current working directory.
  Description:
    Searches for a file in the current working directory.
  Conditions:
    A drive must have been mounted by the FILEIO library.
  Input:
    fileName -   The file's name. May contain limited partial string search
                 elements. '?' can be used as a single\-character wild\-card
                 and '*' can be used as a multiple\-character wild card
                 (only at the end of the file's name or extension).
    attr -       Inclusive OR of all of the attributes (FILEIO_ATTRIBUTES
                 structure members) that a found file may have.
    record -     Structure containing parameters about the found file. Also
                 contains private information used for additional searches
                 for files that match the given criteria in the same
                 directory.
    newSearch -  true if this is the first search for the specified file
                 \parameters in the specified directory, false otherwise.
                 This parameter must be specified as 'true' the first time
                 this function is called with any given FILEIO_SEARCH_RECORD
                 structure. The same FILEIO_SEARCH_RECORD structure should
                 be used with subsequent calls of this function to search
                 for additional files matching the given criteria.
  Return:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
      * Returns file information in the record parameter.
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet Note
        that if the path cannot be resolved, the error will be returned for the
        current working directory.
        * FILEIO_ERROR_INVALID_ARGUMENT - The path could not be
          resolved.
        * FILEIO_ERROR_INVALID_FILENAME - The file name is invalid.
        * FILEIO_ERROR_BAD_CACHE_READ - There was an error searching
          directory entries.
        * FILEIO_ERROR_DONE - File not found.                              
  ******************************************************************************/
int FILEIO_Find (const uint16_t * fileName, unsigned int attr, FILEIO_SEARCH_RECORD * record, bool newSearch);

/***************************************************************************
  Function:
    int FILEIO_LongFileNameGet (FILEIO_SEARCH_RECORD * record, uint16_t * buffer, uint16_t length)

    Summary:
        Obtains the long file name of a file found by the FILEIO_Find 
        function.

    Description:
        This function will obtain the long file name of a file found 
        by the FILEIO_Find function and copy it into a user-specified 
        buffer.  The name will be returned in unicode characters.

    Precondition:
        A drive must have been mounted by the FILEIO library.  The 
        FILEIO_SEARCH_RECORD structure must contain valid file information 
        obtained from the FILEIO_Find function.

    Parameters:
        record - The file record obtained from a successful call of 
            FILEIO_Find.
        buffer - A buffer to contain the long file name of the file.
        length - The length of the buffer, in 16-bit words.

    Returns:
      * If Success: FILEIO_RESULT_SUCCESS
      * If Failure: FILEIO_RESULT_FAILURE
    
      * Sets error code which can be retrieved with FILEIO_ErrorGet Note
        that if the path cannot be resolved, the error will be returned for the
        current working directory.
        * FILEIO_ERROR_INVALID_ARGUMENT - The path could not be
          resolved.
        * FILEIO_ERROR_NO_LONG_FILE_NAME - The short file name does not
          have an associated long file name.
        * FILEIO_ERROR_DONE - The directory entry could not be cached
          because the entryOffset contained in record was invalid.
        * FILEIO_ERROR_WRITE - Cached data could not be written to the
          device.
        * FILEIO_ERROR_BAD_SECTOR_READ - The directory entry could not
          be cached because there was an error reading from the device.                             
  ***************************************************************************************************/
int FILEIO_LongFileNameGet (FILEIO_SEARCH_RECORD * record, uint16_t * buffer, uint16_t length);

/********************************************************************
  Function:
      FILEIO_FILE_SYSTEM_TYPE FILEIO_FileSystemTypeGet (uint16_t driveId)
    
  Summary:
    Describes the file system type of a file system.
  Description:
    Describes the file system type of a file system.
  Conditions:
    A drive must have been mounted by the FILEIO library.
  Input:
    driveId -  Character representation of the mounted device.
  Return:
      * If Success: FILEIO_FILE_SYSTEM_TYPE enumeration member
      * If Failure: FILEIO_FILE_SYSTEM_NONE                          
  ********************************************************************/
FILEIO_FILE_SYSTEM_TYPE FILEIO_FileSystemTypeGet (uint16_t driveId);

/*********************************************************************************
  Function:
    void FILEIO_DrivePropertiesGet()

  Summary:
    Allows user to get the drive properties (size of drive, free space, etc)

   Conditions:
    1) ALLOW_GET_FILEIO_DRIVE_PROPERTIES must be defined in FSconfig.h
    2) a FS_FILEIO_DRIVE_PROPERTIES object must be created before the function is called
    3) the new_request member of the FS_FILEIO_DRIVE_PROPERTIES object must be set before
        calling the function for the first time.  This will start a new search.
    4) this function should not be called while there is a file open.  Close all
        files before calling this function.

  Input:
    properties - a pointer to a FS_FILEIO_DRIVE_PROPERTIES object where the results should
      be stored.

  Return Values:
    This function returns void.  The properties_status of the previous call of 
      this function is located in the properties.status field.  This field has 
      the following possible values:

    FILEIO_GET_PROPERTIES_NO_ERRORS - operation completed without error.  Results
      are in the properties object passed into the function.
    FILEIO_GET_PROPERTIES_DRIVE_NOT_MOUNTED - there is no mounted disk.  Results in
      properties object is not valid
    FILEIO_GET_PROPERTIES_CLUSTER_FAILURE - there was a failure trying to read a 
      cluster from the drive.  The results in the properties object is a partial
      result up until the point of the failure.
    FILEIO_GET_PROPERTIES_STILL_WORKING - the search for free sectors is still in
      process.  Continue calling this function with the same properties pointer 
      until either the function completes or until the partial results meets the
      application needs.  The properties object contains the partial results of
      the search and can be used by the application.  

  Side Effects:
    Can cause errors if called when files are open.  Close all files before
    calling this function.

    Calling this function without setting the new_request member on the first
    call can result in undefined behavior and results.

    Calling this function after a result is returned other than
    FILEIO_GET_PROPERTIES_STILL_WORKING can result in undefined behavior and results.

  Description:  
    This function returns the information about the mounted drive.  The results 
    member of the properties object passed into the function is populated with 
    the information about the drive.    

    Before starting a new request, the new_request member of the properties
    input parameter should be set to true.  This will initiate a new search
    request.

    This function will return before the search is complete with partial results.
    All of the results except the free_clusters will be correct after the first
    call.  The free_clusters will contain the number of free clusters found up
    until that point, thus the free_clusters result will continue to grow until
    the entire drive is searched.  If an application only needs to know that a 
    certain number of bytes is available and doesn't need to know the total free 
    size, then this function can be called until the required free size is
    verified.  To continue a search, pass a pointer to the same FILEIO_FILEIO_DRIVE_PROPERTIES
    object that was passed in to create the search.

    A new search request should be made once this function has returned a value 
    other than FILEIO_GET_PROPERTIES_STILL_WORKING.  Continuing a completed search
    can result in undefined behavior or results.

    Typical Usage:
    <code>
    FILEIO_DRIVE_PROPERTIES disk_properties;

    disk_properties.new_request = true;

    do
    {
        FILEIO_DiskPropertiesGet(&disk_properties, 'A');
    } while (disk_properties.properties_status == FILEIO_GET_PROPERTIES_STILL_WORKING);
    </code>

    results.disk_format - contains the format of the drive.  Valid results are 
      FAT12(1), FAT16(2), or FAT32(3).

    results.sector_size - the sector size of the mounted drive.  Valid values are
      512, 1024, 2048, and 4096.

    results.sectors_per_cluster - the number sectors per cluster.

    results.total_clusters - the number of total clusters on the drive.  This 
      can be used to calculate the total disk size (total_clusters * 
      sectors_per_cluster * sector_size = total size of drive in bytes)

    results.free_clusters - the number of free (unallocated) clusters on the drive.
      This can be used to calculate the total free disk size (free_clusters * 
      sectors_per_cluster * sector_size = total size of drive in bytes)

  Remarks:
    PIC24F size estimates:
      Flash - 400 bytes (-Os setting)

    PIC24F speed estimates:
      Search takes approximately 7 seconds per Gigabytes of drive space.  Speed
        will vary based on the number of sectors per cluster and the sector size.
  *********************************************************************************/
void FILEIO_DrivePropertiesGet (FILEIO_DRIVE_PROPERTIES* properties, uint16_t driveId);

/***************************************************************************
    Function:
        void FILEIO_ShortFileNameGet (FILEIO_OBJECT * filePtr, char * buffer)

    Summary:
        Obtains the short file name of an open file.

    Description:
        Obtains the short file name of an open file.

    Precondition:
        A drive must have been mounted by the FILEIO library and the file
        being specified my be open.

    Parameters:
        filePtr - Pointer to an open file.
        buffer - A buffer to store the null-terminated short file name. Must 
            be large enough to contain at least 13 characters.

    Returns:
        None
***************************************************************************/
void FILEIO_ShortFileNameGet (FILEIO_OBJECT * filePtr, char * buffer);

#endif
