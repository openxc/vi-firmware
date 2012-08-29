/*
* Copyright(C) NXP Semiconductors, 2011
* All rights reserved.
*
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* LPC products.  This software is supplied "AS IS" without any warranties of
* any kind, and NXP Semiconductors and its licensor disclaim any and 
* all warranties, express or implied, including all implied warranties of 
* merchantability, fitness for a particular purpose and non-infringement of 
* intellectual property rights.  NXP Semiconductors assumes no responsibility
* or liability for the use of the software, conveys no license or rights under any
* patent, copyright, mask work right, or any other intellectual property rights in 
* or to any products. NXP Semiconductors reserves the right to make changes
* in the software without notification. NXP Semiconductors also makes no 
* representation or warranty that such application will be suitable for the
* specified use without further testing or modification.
* 
* Permission to use, copy, modify, and distribute this software and its 
* documentation is hereby granted, under NXP Semiconductors' and its 
* licensor's relevant copyrights in the software, without fee, provided that it 
* is used in conjunction with NXP Semiconductors microcontrollers.  This 
* copyright, permission, and disclaimer notice must appear in all copies of 
* this code.
*/


#include "../bsp_internal.h"

#if (BOARD == BOARD_EAOEMBase_RevA)

void bsp_init(void)
{
}

void Serial_Init(const uint32_t BaudRate, const bool DoubleSpeed)
{
#ifdef __LPC177X_8X__
	UART_CFG_Type UARTConfigStruct;

	PINSEL_ConfigPin(UART_PORTNUM,UART_TX_PINNUM,UART_FUNCNUM);
	PINSEL_ConfigPin(UART_PORTNUM,UART_RX_PINNUM,UART_FUNCNUM);
	/* Initialize UART Configuration parameter structure to default state:
	 * Baudrate = 115200bps
	 * 8 data bit
	 * 1 Stop bit
	 * None parity
	 */
	UART_ConfigStructInit(&UARTConfigStruct);
	// Re-configure baudrate
	UARTConfigStruct.Baud_rate = BaudRate;

	// Initialize DEBUG_UART_PORT peripheral with given to corresponding parameter
	UART_Init(DEBUG_UART_PORT, &UARTConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(DEBUG_UART_PORT, ENABLE);
#endif
}

void USB_Connect(void)
{
#ifdef __LPC177X_8X__

#endif
}


void Buttons_Init(void)
{
	GPIO_Init();
	GPIO_SetDir(BUTTONS_BUTTON1_GPIO_PORT_NUM,(1<<BUTTONS_BUTTON1_GPIO_BIT_NUM),0); // input
}

void Joystick_Init(void)
{
	GPIO_SetDir(JOYSTICK_UP_GPIO_PORT_NUM,(1<<JOYSTICK_UP_GPIO_BIT_NUM),0); 		// input
	GPIO_SetDir(JOYSTICK_DOWN_GPIO_PORT_NUM,(1<<JOYSTICK_DOWN_GPIO_BIT_NUM),0); 	// input
	GPIO_SetDir(JOYSTICK_LEFT_GPIO_PORT_NUM,(1<<JOYSTICK_LEFT_GPIO_BIT_NUM),0); 	// input
	GPIO_SetDir(JOYSTICK_RIGHT_GPIO_PORT_NUM,(1<<JOYSTICK_RIGHT_GPIO_BIT_NUM),0); 	// input
	GPIO_SetDir(JOYSTICK_PRESS_GPIO_PORT_NUM,(1<<JOYSTICK_PRESS_GPIO_BIT_NUM),0); 	// input
}

uint8_t Joystick_GetStatus(void)
{
	uint8_t ret = NO_BUTTON_PRESSED;

	if((GPIO_ReadValue(JOYSTICK_UP_GPIO_PORT_NUM) & (1<<JOYSTICK_UP_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= JOY_UP;
	}
	else if((GPIO_ReadValue(JOYSTICK_DOWN_GPIO_PORT_NUM) & (1<<JOYSTICK_DOWN_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= JOY_DOWN;
	}
	else if((GPIO_ReadValue(JOYSTICK_LEFT_GPIO_PORT_NUM) & (1<<JOYSTICK_LEFT_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= JOY_LEFT;
	}
	else if((GPIO_ReadValue(JOYSTICK_RIGHT_GPIO_PORT_NUM) & (1<<JOYSTICK_RIGHT_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= JOY_RIGHT;
	}
	else if((GPIO_ReadValue(JOYSTICK_PRESS_GPIO_PORT_NUM) & (1<<JOYSTICK_PRESS_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= JOY_PRESS;
	}

	return ret;
}
#define AUDIO_MAX_PC	10
uint8_t audio_buffer[2048] __attribute__ ((aligned(4)))__DATA(USBRAM_SECTION);
uint32_t audio_buffer_size = 0;
uint32_t audio_buffer_rd_index = 0;
uint32_t audio_buffer_wr_index = 0;
uint32_t audio_buffer_count = 0;
void Audio_Init(uint32_t samplefreq)
{
	volatile uint32_t pclkdiv, pclk;
	PINSEL_ConfigPin(0, 26, 2);

	PINSEL_SetAnalogPinMode(0,26,ENABLE);
	PINSEL_DacEnable(0, 26, ENABLE);
	  LPC_SC->PCONP |= (1 << 12);
	  LPC_DAC->CR = 0x00008000;		/* DAC Output set to Middle Point */
	  pclkdiv = (LPC_SC->PCLKSEL >> 2) & 0x03;
	  switch ( pclkdiv )
	  {
		case 0x00:
		default:
		  pclk = SystemCoreClock/4;
		break;
		case 0x01:
		  pclk = SystemCoreClock;
		break;
		case 0x02:
		  pclk = SystemCoreClock/2;
		break;
		case 0x03:
		  pclk = SystemCoreClock/8;
		break;
	  }
	  LPC_TIM1->MR0 = pclk/samplefreq - 1;	/* TC0 Match Value 0 */
	  LPC_TIM1->MCR = 3;					/* TCO Interrupt and Reset on MR0 */
	  LPC_TIM1->TCR = 1;					/* TC0 Enable */
	  NVIC_EnableIRQ(TIMER1_IRQn);
	switch(samplefreq){
		case 11025:
		case 22050:
		case 44100:
			audio_buffer_size = 1764;
			break;
		case 8000:
		case 16000:
		case 32000:
		case 48000:
		default:
			audio_buffer_size = samplefreq * 4 * AUDIO_MAX_PC / 1000;
			break;
		}
}
void Audio_Reset_Data_Buffer(void)
{
	audio_buffer_count = 0;
	audio_buffer_wr_index = 0;
	audio_buffer_rd_index = 0;
}
uint32_t Audio_Get_ISO_Buffer_Address(uint32_t last_packet_size)
{
	audio_buffer_wr_index += last_packet_size;
	audio_buffer_count += last_packet_size;

	if(audio_buffer_count > audio_buffer_size)
	{
		Audio_Reset_Data_Buffer();
	}
	if(audio_buffer_wr_index >= audio_buffer_size)
		audio_buffer_wr_index -= audio_buffer_size;
	return (uint32_t)&audio_buffer[audio_buffer_wr_index];
}

void TIMER1_IRQHandler(void)
{
		uint32_t val;
		int32_t sample;
		if(audio_buffer_count >= 2) /*has enough data */
		{
			audio_buffer_count -= 2;
			sample = *(int16_t *)(audio_buffer + audio_buffer_rd_index);
			audio_buffer_rd_index+=2;
			if(audio_buffer_rd_index >= audio_buffer_size)
				audio_buffer_rd_index -= audio_buffer_size;
		}else{
			sample = 0;
		}
		val = sample>>6;
		val &= 0x3FF;
		val += (0x01 << 9);
		DAC_UpdateValue(0, val);
		if((audio_buffer_size != 0) && (audio_buffer_count >= (audio_buffer_size/2)))
		{
			audio_buffer_count-=2;
			audio_buffer_rd_index+=2;
			if(audio_buffer_rd_index >= audio_buffer_size)
				audio_buffer_rd_index -= audio_buffer_size;
		}
		LPC_TIM1->IR = 1;                         /* Clear Interrupt Flag */
}
void LEDs_Init(void){}
void LEDs_SetAllLEDs(uint32_t LEDMask){}

extern uint8_t DiskImage[];
#include "../DataRam.h"
#include <string.h>
/** Initialize the Mass Storage, depend on implementation of board, it can be
 * SPIFI, NOR Flash, NAND Flash, RAM.. */
void MassStorage_Init(void)
{
}

uint32_t MassStorage_Cache_Flush(bool appcall);
/** Program Mass Memory */
void MassStorage_Write(uint32_t diskpos, void* buffer, uint32_t size)
{
	uint32_t block = (uint32_t)diskpos/size;
	if (block < DATA_RAM_PHYSICAL_SIZE/size)
	{
		memcpy(&DiskImage[block*size], buffer, size);
	}
}

void MassStorage_Read(uint32_t diskpos, void* buffer, uint32_t size){
	uint32_t block = (uint32_t)diskpos/size;
	if (block < DATA_RAM_PHYSICAL_SIZE/size)
	{
		memcpy(buffer, &DiskImage[block*size], size);
	}else
		memset(buffer, 0, size);
}

/** Verify Mass Memory with other Memory */
bool MassStorage_Verify(void*dest, void* source, uint32_t size){
	return true;

}
/** Get Mass Storage Size */
uint32_t MassStorage_GetCapacity(void)
{
	return DATA_RAM_VIRTUAL_SIZE;	//use 2 first sectors
}

uint32_t MassStorage_Cache_Flush(bool appcall)
{
	return 0;
}

bool MassStorage_FlushTimeOut(void)
{
	return false;
}
#endif
