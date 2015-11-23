#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <plib.h>
#include "WProgram.h"

#include "fileio.h"
#include "sd_spi.h"
#include "fsman.h"
#include <time.h>

#ifdef RTCC_SUPPORT
	#include "rtcc.h"
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define FS_WRITE_LIMIT 512 //should be 512 or multiple of 512 for optimized disk writes

static uint8_t  fsbuf[FS_WRITE_LIMIT+1];
static uint32_t fsbufptr=0;


static uint32_t fsnameseq=0;
static FILEIO_OBJECT file; //do we want to encapsulate this data into the device structure? Perhaps in a different approach from current
extern void __debug(const char* format, ...);

const char *error_code_str []=
{
    "FILEIO_ERROR_NONE",
    "FILEIO_ERROR_ERASE_FAIL",                    // An erase failed
    "FILEIO_ERROR_NOT_PRESENT",                   // No device was present
    "FILEIO_ERROR_NOT_FORMATTED",                 // The disk is of an unsupported format
    "FILEIO_ERROR_BAD_PARTITION",                 // The boot record is bad
    "FILEIO_ERROR_UNSUPPORTED_FS",                // The file system type is unsupported
    "FILEIO_ERROR_INIT_ERROR",                    // An initialization error has occured
    "FILEIO_ERROR_UNINITIALIZED",                 // An operation was performed on an uninitialized device
    "FILEIO_ERROR_BAD_SECTOR_READ",               // A bad read of a sector occured
    "FILEIO_ERROR_WRITE",                         // Could not write to a sector
    "FILEIO_ERROR_INVALID_CLUSTER",               // Invalid cluster value > maxcls
    "FILEIO_ERROR_DRIVE_NOT_FOUND",               // The specified drive could not be found
    "FILEIO_ERROR_FILE_NOT_FOUND",                // Could not find the file on the device
    "FILEIO_ERROR_DIR_NOT_FOUND",                 // Could not find the directory
    "FILEIO_ERROR_BAD_FILE",                      // File is corrupted
    "FILEIO_ERROR_DONE",                          // No more files in this directory
    "FILEIO_ERROR_COULD_NOT_GET_CLUSTER",         // Could not load/allocate next cluster in file
    "FILEIO_ERROR_FILENAME_TOO_LONG",             // A specified file name is too long to use
    "FILEIO_ERROR_FILENAME_EXISTS",               // A specified filename already exists on the device
    "FILEIO_ERROR_INVALID_FILENAME",              // Invalid file name
    "FILEIO_ERROR_DELETE_DIR",                    // The user tried to delete a directory with FILEIO_Remove
    "FILEIO_ERROR_DELETE_FILE",                   // The user tried to delete a file with FILEIO_DirectoryRemove
    "FILEIO_ERROR_DIR_FULL",                      // All root dir entry are taken
    "FILEIO_ERROR_DRIVE_FULL",                     // All clusters in partition are taken
    "FILEIO_ERROR_DIR_NOT_EMPTY",                 // This directory is not empty yet, remove files before deleting
    "FILEIO_ERROR_UNSUPPORTED_SIZE",              // The disk is too big to format as FAT16
    "FILEIO_ERROR_WRITE_PROTECTED",               // Card is write protected
    "FILEIO_ERROR_FILE_UNOPENED",                 // File not opened for the write
    "FILEIO_ERROR_SEEK_ERROR",                    // File location could not be changed successfully
    "FILEIO_ERROR_BAD_CACHE_READ",                // Bad cache read
    "FILEIO_ERROR_FAT32_UNSUPPORTED",             // FAT 32 - card not supported
    "FILEIO_ERROR_READ_ONLY",                     // The file is read-only
    "FILEIO_ERROR_WRITE_ONLY",                    // The file is write-only
    "FILEIO_ERROR_INVALID_ARGUMENT",              // Invalid argument
    "FILEIO_ERROR_TOO_MANY_FILES_OPEN",           // Too many files are already open
    "FILEIO_ERROR_TOO_MANY_DRIVES_OPEN",          // Too many drives are already open
    "FILEIO_ERROR_UNSUPPORTED_SECTOR_SIZE",       // Unsupported sector size
    "FILEIO_ERROR_NO_LONG_FILE_NAME",             // Long file name was not found
    "FILEIO_ERROR_EOF",                            // End of file reached
};

void _GetTimestamp (struct tm * t){
	
	RTCC_STATUS r = RTCCGetTimeDateDecimal(t);
	if(r != RTCC_NO_ERROR){
		;
	}
}
uint32_t _GetEpochTime(void){

	return ((uint32_t)RTCCGetTimeDateUnix());

}

void Init_RTC(void)
{

	RTCC_STATUS r = I2C_Initialize();
	
	if(r != RTCC_NO_ERROR)
	{
		__debug("RTC_INIT FAILED %d",r);
	}
}

	
// The sdCardMediaParameters structure defines user-implemented functions needed by the SD-SPI fileio driver.
// The driver will call these when necessary.  For the SD-SPI driver, the user must provide
// parameters/functions to define which SPI module to use, Set/Clear the chip select pin,
// get the status of the card detect and write protect pins, and configure the CS, CD, and WP
// pins as inputs/outputs (as appropriate).
// For this demo, these functions are implemented in system.c, since the functionality will change
// depending on which demo board/microcontroller you're using.
// This structure must be maintained as long as the user wishes to access the specified drive.

FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters =
{
    SPI_MDD_CHANNEL_NO,                 // Use SPI module 2
    USER_SdSpiSetCs_2,                  // User-specified function to set/clear the Chip Select pin.
    USER_SdSpiGetCd_2,                  // User-specified function to get the status of the Card Detect pin.
    USER_SdSpiGetWp_2,                  // User-specified function to get the status of the Write Protect pin.
    USER_SdSpiConfigurePins_2           // User-specified function to configure the pins' TRIS bits.
};


// The gSDDrive structure allows the user to specify which set of driver functions should be used by the
// FILEIO library to interface to the drive.
// This structure must be maintained as long as the user wishes to access the specified drive.
const FILEIO_DRIVE_CONFIG gSdDrive =
{
    (FILEIO_DRIVER_IOInitialize)FILEIO_SD_IOInitialize,                     // Function to initialize the I/O pins used by the driver.
    (FILEIO_DRIVER_MediaDetect)FILEIO_SD_MediaDetect,                       // Function to detect that the media is inserted.
    (FILEIO_DRIVER_MediaInitialize)FILEIO_SD_MediaInitialize,               // Function to initialize the media.
    (FILEIO_DRIVER_MediaDeinitialize)FILEIO_SD_MediaDeinitialize,           // Function to de-initialize the media.
    (FILEIO_DRIVER_SectorRead)FILEIO_SD_SectorRead,                         // Function to read a sector from the media.
    (FILEIO_DRIVER_SectorWrite)FILEIO_SD_SectorWrite,                       // Function to write a sector to the media.
    (FILEIO_DRIVER_WriteProtectStateGet)FILEIO_SD_WriteProtectStateGet,     // Function to determine if the media is write-protected.
};

void GetTimestamp (FILEIO_TIMESTAMP * timeStamp)
{
	struct tm ts;
	FILEIO_DATE *d = &timeStamp->date;
	FILEIO_TIME *t = &timeStamp->time;
	
	_GetTimestamp(&ts);
	
	d->bitfield.day   = ts.tm_mday;
	d->bitfield.month = ts.tm_mon;
	
	if(ts.tm_year > 1980)
		d->bitfield.year  = ts.tm_year-1980;
	else
		d->bitfield.year  = 0;
	
	t->bitfield.secondsDiv2 = ts.tm_sec/2;
	t->bitfield.minutes		= ts.tm_min;
	t->bitfield.hours	    = ts.tm_hour;
	
    timeStamp->timeMs = (uint8_t)millis() & 0xFF; //todo get a reference time here
}


const char* fsmanGetErrStr(uint8_t code){
	
	if (code > FILEIO_ERROR_EOF){
		
		return "ERR OB";
	}

	return error_code_str[code];

}

uint8_t fsmanMountSD(uint8_t * result_code){
	
    if (*result_code = FILEIO_DriveMount('A', &gSdDrive, &sdCardMediaParameters), 
			*result_code > 0){
			
        return FALSE;
    } 
	return TRUE;
       
}
void fsmanInitHardwareSD(void)
{
	__debug("Initializing CS GPIO");
	SYSTEM_Initialize();
}



uint8_t fsmanInit(uint8_t * result_code){
	
	uint8_t file_name[25];
	
	fsmanInitHardwareSD();

	//Initialize RTCC module for filetimestamping
	Init_RTC();

	memset(&file,0, sizeof(FILEIO_OBJECT));
	 
    if (!FILEIO_Initialize()){
		
        *result_code = FILEIO_ERROR_INIT_ERROR;//from FILEIO_ERROR_TYPE
		return 0;
    }
    // Register the GetTimestamp function as the timestamp source for the library.
    FILEIO_RegisterTimestampGet	(GetTimestamp);
	__debug("FILE Media Detect");
	if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) != TRUE){
		*result_code = FILEIO_ERROR_INIT_ERROR;//from FILEIO_ERROR_TYPE
        return 0; 
    }
	__debug("Mount SD Card");
	if(fsmanMountSD(result_code) == FALSE){
		
		__debug("Mount Fail");
		return 0;
	}
	//Create a VI_LOG directory if there isnt already one
	
	if(FILEIO_DirectoryChange ("VI_LOG") != FILEIO_RESULT_SUCCESS){
		__debug("VI_LOG Directory was not found, creating it");	
		if(FILEIO_DirectoryMake ("VI_LOG") != FILEIO_RESULT_SUCCESS){
			__debug("Unable to create a VI_LOG directory");
			return 0;
		}
		else{
			if(FILEIO_DirectoryChange ("VI_LOG") != FILEIO_RESULT_SUCCESS){
				__debug("VI_LOG Directory was created but not found");
				return 0;
			}
		}
	}
	return 1;
}

uint8_t fsmanUnmountSD(uint8_t * result_code)
{
	//unmount drive
	__debug("Unmounting Drive");
	if(*result_code = FILEIO_DriveUnmount ('A'), 
			*result_code != FILEIO_RESULT_SUCCESS){
		return FALSE;		
	}
	return TRUE;
}

uint8_t fsmanDeInit(uint8_t * result_code){
	
	//Todo Reset and release SPI for SD Card
	return fsmanUnmountSD(result_code);;
	
}
uint8_t fsmanSessionIsActive(void){
	if(file.disk == NULL){//File does not exist
			return FALSE;
	}
	return TRUE;
}


uint8_t fsmanSessionStart(uint8_t * result_code){
	//open file here
	char file_name[25];
	uint32_t tm_code;
	
	tm_code = _GetEpochTime();
	
	sprintf(file_name,"%X.TXT", tm_code);
							
	__debug("Creating %s",file_name);
	
	if (*result_code = FILEIO_Open (&file, file_name, FILEIO_OPEN_WRITE | FILEIO_OPEN_CREATE), 
			*result_code != FILEIO_RESULT_FAILURE){
		//perhaps do some memory clean ups here
		fsbufptr = 0;
		return TRUE;
       
    }
	return FALSE;
	
}

uint32_t fsmanSessionCacheBytesWaiting(void){
	return fsbufptr;
}

uint8_t fsmanSessionReset(uint8_t * result_code){
	
	uint32_t pending = fsmanSessionCacheBytesWaiting();
	if(pending){
		__debug("Writing to disk %d bytes", pending);
		if (FILEIO_Write (fsbuf, 1, pending, &file) != pending){
			*result_code = FILEIO_ERROR_WRITE;
			return FALSE;
		} else{
			fsbufptr = 0;
		}
	}
	if (*result_code = FILEIO_Close (&file),
		*result_code != FILEIO_RESULT_SUCCESS){
        return FALSE;
    }
	return fsmanSessionStart(result_code);
}
uint8_t fsmanLogBuffer(uint8_t* fsbuf, uint32_t sz){
	uint8_t sb[65];
	uint32_t i;
	
	__debug("Printing File Buffer %d size", sz);
	for(i=0; i<sz; i++)
	{
		memcpy(sb,&fsbuf[i], 64);
		i+=64;
		sb[64] = 0;
		__debug("%s",sb);
	}
	return 0;
}

uint8_t fsmanSessionWrite(uint8_t * result_code, uint8_t* data, uint32_t len){
	

	//read and store 512 bytes after which you should probably write
	uint32_t sz = MIN(len, FS_WRITE_LIMIT - fsbufptr);

	memcpy(&fsbuf[fsbufptr],data, sz);
	fsbufptr += sz;
	

	if (fsbufptr >= FS_WRITE_LIMIT){ //todo add a time limit so that we do exceed file size limits
		__debug("Writing to disk %d bytes", FS_WRITE_LIMIT);

		if (FILEIO_Write (fsbuf, 1, FS_WRITE_LIMIT, &file) != FS_WRITE_LIMIT){
			*result_code = FILEIO_ERROR_WRITE;
			return FALSE;
		} else{
			memcpy(fsbuf,&data[sz], len-sz); //copy pending data if any
			fsbufptr = len-sz;
		}
	}
	*result_code = FILEIO_ERROR_NONE;
	return TRUE;
}
uint8_t fsmanSessionFlush(uint8_t * result_code){

	if(*result_code = FILEIO_Flush(&file), 
		*result_code != FILEIO_RESULT_SUCCESS){ //writes unwritten data to flash without closing the file
		
		return FALSE;
	}
	return TRUE;
}

uint8_t fsmanSessionEnd(uint8_t * result_code){
	
	if (*result_code = FILEIO_Close (&file), 
			*result_code != FILEIO_RESULT_SUCCESS){
			
		return FALSE;
    }
	if(!fsmanDeInit(result_code)){
		return FALSE;
	}
	
	file.disk = NULL;
	return TRUE;
}


























