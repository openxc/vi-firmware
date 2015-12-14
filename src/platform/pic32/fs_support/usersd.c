#include <stdbool.h>
#include <stdint.h>
#include <plib.h>
inline uint8_t User_MDD_SDSPI_MediaDetect(void){
    //Soft detect used
    #ifdef CROSSCHASM_C5_BT
        return 1; //Todo should we read SD status?
    #elif CROSSCHASM_C5_CELLULAR
        return(PORTBbits.RB5);    
    #else
        #error "Invalid platform define CD"
    #endif
}
inline uint8_t User_MDD_SDSPI_WriteProtectState(void){
    return 0;
}

void User_MDD_SDSPI_IO_Init(void){ 

#ifdef CROSSCHASM_C5_CELLULAR   
   TRISBSET = (1 << 5);
#endif

}
