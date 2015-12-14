#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include <plib.h>

bool SPI_Open(uint8_t channel/*expose spi options as needed*/) {


    if(!VerifyValidSpiChannel(channel)){
        return false;
    }
    static uint32_t config = SPICON_MSTEN |         // master mode enable
                             //SPICON_CKP |
                             //SPICON_SSEN |
                             SPICON_CKE |
                             SPICON_SMP |           // input sample at end of sample period
                             //SPICON_MODE32 |
                             //SPICON_DISSDO |
                             SPICON_SIDL |          // stop in IDLE
                             //SPICON_FRZ |
                             SPICON_ON              // peripheral ON
                             //SPICON_SPIFE |
                             //SPICON_FRMPOL |
                             //SPICON_FRMSYNC |
                             //SPICON_FRMEN
                             ;
    
    // BR = Fpb/fpbDiv;
    // BR = Fpb/(2*(SPIBRG+1))
    //uint32_t fpbDiv = 40; //2Mhz
      uint32_t fpbDiv = 10; //8Mhz
      
    SpiChnOpen(channel, config, fpbDiv);

    return true;

}

void SPI_Close(uint8_t channel) {

    if(!VerifyValidSpiChannel(channel)){
        return;
    }

    SpiChnClose(channel);
    
}

uint8_t SPI_SendByte(uint8_t channel, uint8_t byte) {

    //if(!VerifyValidSpiChannel(channel)){
    //    return NULL;
   // }

    // block if buf is full
   // while(!SpiChnTxBuffEmpty(channel));
    // write
    //SpiChnPutC(channel, byte);
    SPI2BUF = byte;
    while(!SPI2STATbits.SPIRBF);        
    // block until complete
    //while(!SpiChnDataRdy(channel));

    // read dummy
    //return SpiChnGetC(channel);
    return SPI2BUF;

}

uint8_t SPI_ReceiveByte(uint8_t channel) {

   // if(!VerifyValidSpiChannel(channel)){
   //     return NULL;
   // }    
    // clock (dummy write)
    //SpiChnPutC(channel, 0xFF);
    SPI2BUF = 0xFF;
    while(!SPI2STATbits.SPIRBF);    

    // wait for data
    //while(!SpiChnDataRdy(channel));

    // read and return
    //return (uint8_t)SpiChnGetC(channel);
    return SPI2BUF;

}


bool VerifyValidSpiChannel(uint8_t channel){
/*
    // PIC32 only supporting 4 SPI channels.
    if (channel < 1 || channel > 4){
        return false;
    }
    // Check if the board has support for that channel
    if(((1 << (channel-1)) && ActiveSPIChannels) != 1){
        return false;
    }
 * */
    return true;
}

void SPI_CS_Enable(uint8_t channel){
 
    if( channel == 2){
        CS_SPICHANNEL2_ENABLE();
        CS_SPICHANNEL2_ENABLE();
        CS_SPICHANNEL2_ENABLE();
        CS_SPICHANNEL2_ENABLE();
    }
}

void SPI_CS_Disable(uint8_t channel){
    if( channel == 2){
        CS_SPICHANNEL2_DISABLE();
        CS_SPICHANNEL2_DISABLE();
        CS_SPICHANNEL2_DISABLE();
        CS_SPICHANNEL2_DISABLE();
           
    }
}

void SPI_CS_SetHigh(uint8_t channel){
    if( channel == 2){
        CS_SPICHANNEL2_ON();
        CS_SPICHANNEL2_ON();
        CS_SPICHANNEL2_ON();
        CS_SPICHANNEL2_ON();
        CS_SPICHANNEL2_ON();
            
    }
}

void SPI_CS_SetLow(uint8_t channel){
    if( channel == 2){
        CS_SPICHANNEL2_OFF();
        CS_SPICHANNEL2_OFF();
        CS_SPICHANNEL2_OFF(); 
        CS_SPICHANNEL2_OFF(); 
    }
}