#include "platform_profile.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <plib.h>
#include "WProgram.h"
#include "usersd.h"
#include "rtc.h"

#ifdef FS_SUPPORT

#include "MDD File System/FSIO.h"
#include "fsman.h"
#include <time.h>

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


static uint8_t* fsbuf;
static uint32_t fsbufptr=0;

static uint32_t fsnameseq=0;

static FSFILE* file = NULL; //do we want to encapsulate this data into the device structure? Perhaps in a different approach from current

const char *error_code_str []=
{
    "CE_GOOD",                          // No error
    "CE_ERASE_FAIL",                  // An erase failed
    "CE_NOT_PRESENT",                 // No device was present
    "CE_NOT_FORMATTED",               // The disk is of an unsupported format
    "CE_BAD_PARTITION",               // The boot record is bad
    "CE_UNSUPPORTED_FS",              // The file system type is unsupported
    "CE_INIT_ERROR",                  // An initialization error has occured
    "CE_NOT_INIT",                    // An operation was performed on an uninitialized device
    "CE_BAD_SECTOR_READ",             // A bad read of a sector occured
    "CE_WRITE_ERROR",                 // Could not write to a sector
    "CE_INVALID_CLUSTER",             // Invalid cluster value > maxcls
    "CE_FILE_NOT_FOUND",              // Could not find the file on the device
    "CE_DIR_NOT_FOUND",               // Could not find the directory
    "CE_BAD_FILE",                    // File is corrupted
    "CE_DONE",                        // No more files in this directory
    "CE_COULD_NOT_GET_CLUSTER",       // Could not load/allocate next cluster in file
    "CE_FILENAME_2_LONG",             // A specified file name is too long to use
    "CE_FILENAME_EXISTS",             // A specified filename already exists on the device
    "CE_INVALID_FILENAME",            // Invalid file name
    "CE_DELETE_DIR",                  // The user tried to delete a directory with FSremove
    "CE_DIR_FULL",                    // All root dir entry are taken
    "CE_DISK_FULL",                   // All clusters in partition are taken
    "CE_DIR_NOT_EMPTY",               // This directory is not empty yet, remove files before deleting
    "CE_NONSUPPORTED_SIZE",           // The disk is too big to format as FAT16
    "CE_WRITE_PROTECTED",             // Card is write protected
    "CE_FILENOTOPENED",               // File not opened for the write
    "CE_SEEK_ERROR",                  // File location could not be changed successfully
    "CE_BADCACHEREAD",                // Bad cache read
    "CE_CARDFAT32",                   // FAT 32 - card not supported
    "CE_READONLY",                    // The file is read-only
    "CE_WRITEONLY",                   // The file is write-only
    "CE_INVALID_ARGUMENT",            // Invalid argument
    "CE_TOO_MANY_FILES_OPEN",         // Too many files are already open
    "CE_UNSUPPORTED_SECTOR_SIZE"      // Unsupported sector size  // End of file reached
};


// internal errors
#define CE_FAT_EOF              60   // Error that indicates an attempt to read FAT entries beyond the end of the file
#define CE_EOF                  61   // Error that indicates that the end of the file has been reached

#define UNKNOWN_SD_MOUNT_ERROR  90
#define UNKNOWN_WRITE_ERROR     91



const char* fsmanGetErrStr(uint8_t code){
    
    if (code > CE_UNSUPPORTED_SECTOR_SIZE){ //todo check for EOF
        
        return "ERR OB";
    }
    return error_code_str[code];
}

uint8_t fsmanMountSD(uint8_t * result_code){
    
    uint8_t ret=0;
    
    MEDIA_INFORMATION  *mediainfo = MDD_MediaInitialize();
    
    if(mediainfo->errorCode != MEDIA_NO_ERROR){
        __debug("MDD media init failed");
    }
    
    if(MDD_MediaDetect() == 0){
        __debug("MDD media detect failed");
        return 0;
    }
    
    if(FSInit() == 0){
        __debug("FSInit failed");
        return 0;
    }

    return 1;
       
}
void fsmanInitHardwareSD(void)
{
    __debug("Initializing CS GPIO");
    User_MDD_SDSPI_IO_Init();
    MDD_SDSPI_InitIO();
}


uint8_t fsmanFormat(void){

    __debug("Formatting Drive");
    uint8_t res =  FSformat (0, 0, "OPENXC");
    __debug("Formatting Complete %d",res);
    if(res == CE_GOOD)
        return 1;
    
    return 0;
}

uint8_t fsmanInit(uint8_t * result_code, uint8_t* buffer){
    
    struct tm ts;
    
    fsbuf =  buffer;
    RTC_GetTimeDateDecimal(&ts);
    SetClockVars (ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
    
    fsmanInitHardwareSD();
    if (!fsmanMountSD(result_code)){
        
        *result_code = UNKNOWN_SD_MOUNT_ERROR;//from FILEIO_ERROR_TYPE
        return 0;
    }

    //Create a VI_LOG directory if there isnt already one

    if(FSchdir("VI_LOG") != CE_GOOD){
        __debug("VI_LOG Directory was not found, creating it");    
        if(FSmkdir(".\\VI_LOG") != CE_GOOD){
            __debug("Unable to create a VI_LOG directory");
             *result_code = UNKNOWN_WRITE_ERROR;
            return 0;
        }
        else{
            if(FSchdir ("VI_LOG") != CE_GOOD){
                *result_code = UNKNOWN_WRITE_ERROR;
                __debug("VI_LOG Directory was created but not found");
                return 0;
            }
        }
    }
    return 1;
}

uint8_t fsmanUnmountSD(uint8_t * result_code)
{
    //nothing else to do at this point
    __debug("Unmounting Drive");
    return TRUE;
}

uint8_t fsmanDeInit(uint8_t * result_code){
    
    //Todo Reset and release SPI for SD Card
    return fsmanUnmountSD(result_code);;
    
}
uint8_t fsmanSessionIsActive(void){
    
    if(file == NULL){//File does not exist
            return FALSE;
    }
    return TRUE;
}


uint8_t fsmanSessionStart(uint8_t * result_code){
    //open file here
    char file_name[25];
    struct tm ts;
    
    uint32_t tm_code;
    
    tm_code =  (uint32_t)RTC_GetTimeDateUnix();
    
    RTC_GetTimeDateDecimal(&ts);
    
    SetClockVars (ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
    
    sprintf(file_name,"%X.TXT", tm_code);
                            
    __debug("Creating %s",file_name);
    
    file = FSfopen (file_name,"w");
    
    if (file == NULL){
        *result_code = UNKNOWN_WRITE_ERROR;
        return FALSE;
    }
    return TRUE;
}

uint32_t fsman_available(void){
    return (FS_BUF_SZ - fsbufptr);
}


uint32_t fsmanSessionCacheBytesWaiting(void){
    
    return fsbufptr;
}

uint8_t fsmanSessionReset(uint8_t * result_code){
    
    uint32_t pending = fsmanSessionCacheBytesWaiting();
    if(pending){
        __debug("Resetting session pending to disk %d bytes", pending);
        
        if (FSfwrite ((void *)fsbuf, 1, pending, file) != pending){
            *result_code = UNKNOWN_WRITE_ERROR;
            __debug("Write pending bytes failed");
            file = NULL;
            return FALSE;
        }else{
            fsbufptr = 0;
        }
    }
    if (*result_code = FSfclose(file),
            *result_code != CE_GOOD){
        return FALSE;
    }
    return fsmanSessionStart(result_code);
}
uint8_t fsmanSessionWrite(uint8_t * result_code, uint8_t* data, uint32_t len){
    
    uint32_t sz = MIN(len, FS_BUF_SZ - fsbufptr);
    memcpy(&fsbuf[fsbufptr],data, sz);
    fsbufptr += sz;
    if (fsbufptr >= FS_BUF_SZ){ //todo add a time limit so that we do exceed file size limits
        if ( FSfwrite(fsbuf, 1, FS_BUF_SZ, file) != FS_BUF_SZ){
            *result_code = UNKNOWN_WRITE_ERROR;
            return FALSE;
        } else{
            memcpy(fsbuf,&data[sz], len-sz); //copy pending data if any
            fsbufptr = len-sz;
            __debug("Wrote %d bytes",FS_BUF_SZ);
        }
    }
    *result_code = CE_GOOD;
    return TRUE;
}

uint8_t fsmanSessionFlush(uint8_t * result_code){

    if(*result_code = FSfflush(file), 
        *result_code != CE_GOOD){ //writes unwritten data to flash without closing the file
        
        return FALSE;
    }
    return TRUE;
}

uint8_t fsmanSessionEnd(uint8_t * result_code){
    
    if (*result_code = FSfclose (file), 
            *result_code != CE_GOOD){
            
        return FALSE;
    }
    if(!fsmanDeInit(result_code)){
        return FALSE;
    }
    
    file = NULL;
    return TRUE;
}

#endif
























