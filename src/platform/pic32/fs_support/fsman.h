#ifndef __FSMAN_H
#define __FSMAN_H

#ifdef    __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
        
const char* fsmanGetErrStr(uint8_t code);
uint8_t fsmanMountSD     (uint8_t * result_code);
uint8_t fsmanInit(uint8_t * result_code, uint8_t* buffer);
uint8_t fsmanFormat         (void);
uint8_t fsmanUnmountSD   (uint8_t * result_code);
uint8_t fsmanDeInit      (uint8_t * result_code);
uint8_t fsmanSessionWrite(uint8_t * result_code, uint8_t* data, uint32_t len);
uint8_t fsmanSessionReset(uint8_t * result_code);
uint8_t fsmanSessionFlush(uint8_t * result_code);
uint8_t fsmanSessionIsActive(void);
uint8_t fsmanSessionStart(uint8_t * result_code);
uint8_t fsmanSessionEnd(uint8_t * result_code);
uint32_t fsmanSessionCacheBytesWaiting(void);
void fsmanInitHardwareSD(void);
uint32_t fsman_available(void);
#ifdef    __cplusplus
}
#endif


#endif