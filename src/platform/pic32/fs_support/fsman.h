#ifndef __FSMAN_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
        
#define SPI_MDD_CHANNEL_NO			   2

#define CS_SPICHANNEL2_ENABLE()         LATESET  = (1 << 4); TRISECLR = (1 << 4)
#define CS_SPICHANNEL2_DISABLE()        TRISESET = (1 << 4)

#define CS_SPICHANNEL2_ON()             LATESET  = (1 << 4)
#define CS_SPICHANNEL2_OFF()            LATECLR  = (1 << 4)

#define CS_SD_ENABLE()      TRISGCLR = (1 << 9)
#define CS_SD_LOW()         LATGCLR  = (1 << 9)
#define CS_SD_HIGH()        LATGSET  = (1 << 9) 

const char* fsmanGetErrStr(uint8_t code);

uint8_t fsmanMountSD     (uint8_t * result_code);
uint8_t fsmanInit        (uint8_t * result_code);
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
#ifdef	__cplusplus
}
#endif


#endif