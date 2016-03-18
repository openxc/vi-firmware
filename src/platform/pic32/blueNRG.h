#ifndef __BLUENRG_H_
#define __BLUENRG_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*BTLE SPI*/
#define STBTLE_SPICHANNEL 2

/*BTLE RST IO*/
#define SPBTLE_RST_CONFIGURE()         TRISECLR = (1 << 3)
#define SPBTLE_RST_ENABLE()          LATECLR  = (1 << 3)
#define SPBTLE_RST_DISABLE()         LATESET  = (1 << 3)


extern uint8_t stickyfisr;

void BlueNRG_Hal_Write_Serial(const void* data1, const void* data2, uint16_t n_bytes1, uint16_t n_bytes2);

BOOL BlueNRG_DataPresent(void);

int8_t BlueNRG_SpiInit(void);
 
int  BlueNRG_ISRInit(void);

int BlueNRG_ISRDeInit(void);

void BlueNRG_Clear_IRQ_Pending(void);

void BlueNRG_SPI_IRQ_Suspend(void);

void BlueNRG_SPI_IRQ_Engage(void);

uint8_t BlueNRG_SPI_Read_All(uint8_t *buffer, uint16_t buff_size);

void BlueNRG_RST(void);

void BlueNRG_PowerOff(void);


#ifdef __cplusplus
}
#endif

#endif



 