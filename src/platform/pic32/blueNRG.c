
/* Includes ------------------------------------------------------------------*/


#include <stdint.h>
#include <stdbool.h>
#include <plib.h>
#include "spi.h" 
#include "blueNRG.h"
#include "hci.h"
#include "WProgram.h" //for arduino millis  reference
#include "platform_profile.h"
/**
 * @}
 */

/** @defgroup SPI_Private_Variables
* @{
*/

/**
* @}
*/

/** @defgroup SPI_Functions
* @{
*/
uint8_t stickyfisr = 0;

#ifdef BLE_SUPPORT
void __ISR(_EXTERNAL_0_VECTOR, ipl7) INT0Interrupt() 
{ 
    
    mINT0ClearIntFlag(); 
    if(BlueNRG_DataPresent()) //add a small amount of debounce
    {
        if(BlueNRG_DataPresent()) 
        {
            HCI_Isr();         
        }
    }
    
} 
#endif


void BlueNRG_SPI_IRQ_Suspend(void){
    DisableINT0;
}

void BlueNRG_SPI_IRQ_Engage(void){
    EnableINT0;
}

BOOL BlueNRG_DataPresent(void) {
  if( PORTDbits.RD0 > 0) return TRUE;    
  return FALSE;  
}

/**
* @brief  Initializes the SPI 
* @param  none
* @retval status
*/
int8_t BlueNRG_SpiInit(void) {
  SPI_Open(STBTLE_SPICHANNEL);
  SPI_CS_Enable(STBTLE_SPICHANNEL);
  SPI_CS_SetHigh(STBTLE_SPICHANNEL);
  return(0);
}/* end BlueNRGSpiInit() */


int BlueNRG_ISRDeInit(void){
    BlueNRG_SPI_IRQ_Suspend();//should we float input?
}

int BlueNRG_ISRInit(void){
    TRISDSET = (1 << 0);
    LATDSET  = (1 << 0);
    ConfigINT0(EXT_INT_ENABLE | RISING_EDGE_INT | EXT_INT_PRI_7);
    return (0);
}/* end BlueNRGISRInit() */


static void SPI_SendRecieve(uint8_t channel, uint8_t * sendb, uint8_t * recvb, uint32_t n ){
   uint32_t i;  
   if(sendb == NULL && recvb == NULL)
       return;
   if(sendb == NULL && recvb != NULL)
   {
       for( i=0 ; i < n ; i++)
       {
            recvb[i] = SPI_SendByte(channel, 0xFF);
       }
   }
   else if (sendb != NULL && recvb != NULL)
   {
      for( i=0 ; i < n ; i++)
      {
        recvb[i] = SPI_SendByte(channel, sendb[i]); //send receive SPI information
      }
   }
   else
   {
      for( i=0 ; i < n ; i++)
      {
        SPI_SendByte(channel, sendb[i]); 
      }
   }
}

/**
* @brief  Read from BlueNRG SPI buffer and store data into local buffer 
* @param  buffer:    buffer where data from SPI are stored
*         buff_size: buffer size
* @retval number of read bytes
*/
uint8_t BlueNRG_SPI_Read_All(uint8_t *buffer, uint16_t buff_size)
{
  uint16_t byte_count;
  uint8_t len = 0;
  
  uint8_t header_master[5] = {0x0b, 0x00, 0x00, 0x00, 0x00};//TODO implement 32 bit*4 FIFO based transfers for SPI
  uint8_t header_slave[5];
  
  SPI_CS_SetLow(STBTLE_SPICHANNEL);
 
  //read the header
  SPI_SendRecieve(STBTLE_SPICHANNEL, header_master, header_slave, 5);
  
  if (header_slave[0] == 0x02) {
    // device is ready
    
    byte_count = (header_slave[4]<< 8 ) | header_slave[3];
    
    if (byte_count > 0) {
      
      // avoid to read more data that size of the buffer
      if (byte_count > buff_size)
      {  
        byte_count = buff_size;
      }
      
      SPI_SendRecieve(STBTLE_SPICHANNEL, NULL, buffer, byte_count );
    
      len = byte_count;
    }    
  }
  SPI_CS_SetHigh(STBTLE_SPICHANNEL);
   
  return len;
  
}/* end BlueNRG_SPI_Read_All() */


/**
* @brief  Write data from local buffer to SPI
* @param  data1:    first data buffer to be written, used to send header of higher
*                   level protocol
*         data2:    second data buffer to be written, used to send payload of higher
*                   level protocol
*         Nb_bytes1: size of header to be written
*         Nb_bytes2: size of payload to be written
* @retval Number of payload bytes that has been sent. If 0, all bytes in the header has been
*         written.
*/
static int16_t SPI_Write(uint8_t* data1, uint8_t* data2, uint16_t Nb_bytes1, uint16_t Nb_bytes2)
{  
  int16_t result = 0;
  uint16_t tx_bytes;
  uint8_t rx_bytes;
  
  uint8_t header_master[5] = {0x0a, 0x00, 0x00, 0x00, 0x00};
  
  uint8_t header_slave[5]  = {0x00};
  
  SPI_CS_SetLow(STBTLE_SPICHANNEL);
   
  SPI_SendRecieve(STBTLE_SPICHANNEL, header_master, header_slave, 5);
  
  if(header_slave[0] != 0x02){
    result = -1;
    goto failed; // BlueNRG not awake.
  }
  
  rx_bytes = header_slave[1];
  
  if(rx_bytes < Nb_bytes1)
  {
    result = -2;
    goto failed; // underflow      
  }
  
  SPI_SendRecieve(STBTLE_SPICHANNEL, data1, NULL, Nb_bytes1);
  
  rx_bytes -= Nb_bytes1;
  
  if(Nb_bytes2 > rx_bytes)
  {  
    tx_bytes = rx_bytes;
  }
  else
  {
    tx_bytes = Nb_bytes2;
  }
  
  SPI_SendRecieve(STBTLE_SPICHANNEL, data2, NULL, tx_bytes);

  result = tx_bytes; //return how much payload was sent
  
failed:
  
  // Release CS line
  SPI_CS_SetHigh(STBTLE_SPICHANNEL);
  
  return result;
}/* end BlueNRG_SPI_Write() */




void BlueNRG_Hal_Write_Serial(const void* data1, const void* data2, uint16_t n_bytes1, uint16_t n_bytes2)
{

  int ret;
  uint8_t data2_offset = 0;
  uint32_t tm;
  
  tm = millis() + 1000;
  
  while(1){
      
    ret = SPI_Write((uint8_t *)data1,(uint8_t *)data2 + data2_offset, n_bytes1, n_bytes2); 
    
    if(ret >= 0)
    {      
      n_bytes1 = 0;
      n_bytes2 -= ret;
      
      data2_offset += ret;
      if(n_bytes2==0)
        break;
    }

    if(millis() > tm)
    {
      break; //timeout on data handled by higher level application
    }
  }
}

void BlueNRG_DelayMS(uint32_t d)
{
    uint32_t tm;
    tm = millis() + d;            //todo create a microsecond differential coretick timer delay
    while(millis() < tm);        //todo if millis does not increase will result in halt
    
}

void BlueNRG_RST(void)
{
    SPBTLE_RST_ENABLE();      
    BlueNRG_DelayMS(5);
    SPBTLE_RST_DISABLE();      
    BlueNRG_DelayMS(5);
    
}

void BlueNRG_PowerOff(void)
{
    SPBTLE_RST_ENABLE();  
    BlueNRG_DelayMS(5);
}
