#ifndef __FS_PLATFORMS_H_
#define __FS_PLATFORMS_H_

#if (defined(CROSSCHASM_C5_BT) || defined(CROSSCHASM_C5_CELLULAR))
    #if defined __MSD_ENABLE__
        #define FS_SUPPORT
    #endif
#endif

    #define FS_BUF_SZ 2048 
    
    #ifndef DEFAULT_FILE_GENERATE_SECS
        #define DEFAULT_FILE_GENERATE_SECS 180
        #warning "DEFAULT_FILE_GENERATE_SECS=180 applied"
    #endif
    
    #if DEFAULT_FILE_GENERATE_SECS < 15
        #undef DEFAULT_FILE_GENERATE_SECS
        #define DEFAULT_FILE_GENERATE_SECS 180
        #warning "DEFAULT_FILE_GENERATE_SECS=180 applied"
    #endif
    
    #define FILE_WRITE_RATE_SEC     DEFAULT_FILE_GENERATE_SECS
    
    #define FILE_FLUSH_DATA_TIMEOUT_SEC 60

#endif