/* 
 * File:   spi.h
 * Author: Michael
 *
 * Created on May 3, 2015, 6:33 PM
 */

#ifndef SPI_H
#define    SPI_H

#ifdef    __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

//ACI_CS RE4


#define CS_SPICHANNEL2_ENABLE()         LATESET  = (1 << 4); TRISECLR = (1 << 4)
#define CS_SPICHANNEL2_DISABLE()        TRISESET = (1 << 4)

#define CS_SPICHANNEL2_ON()             LATESET  = (1 << 4)
#define CS_SPICHANNEL2_OFF()            LATECLR  = (1 << 4)



bool SPI_Open(uint8_t channel/*expose spi options as needed*/);
void SPI_Close(uint8_t channel);
uint8_t SPI_SendByte(uint8_t channel, uint8_t byte);
uint8_t SPI_ReceiveByte(uint8_t channel);
void SPI_CS_Enable(uint8_t channel);
void SPI_CS_Disable(uint8_t channel);
void SPI_CS_SetHigh(uint8_t channel);
void SPI_CS_SetLow(uint8_t channel);
bool VerifyValidSpiChannel(uint8_t channel);

#ifdef    __cplusplus
}
#endif

#endif    /* SPI_H */

