#ifndef __USERSD_H
#define __USERSD_H

#ifdef    __cplusplus
extern "C" {
#endif

void User_MDD_SDSPI_IO_Init(void);
inline uint8_t User_MDD_SDSPI_MediaDetect(void);
inline uint8_t User_MDD_SDSPI_WriteProtectState(void);

#ifdef    __cplusplus
}
#endif


#endif