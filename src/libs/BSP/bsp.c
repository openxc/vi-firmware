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


#include "bsp_internal.h"

/**
 * LED API
 */
void LEDs_TurnOnLEDs(uint32_t LEDMask) {}
void LEDs_TurnOffLEDs(uint32_t LEDMask) {}
void LEDs_ChangeLEDs(uint32_t LEDMask, uint32_t ActiveMask) {}
void LEDs_ToggleLEDs(uint32_t LEDMask) {}
uint32_t LEDs_GetLEDs(void) { return 0; }

/**
 * JoyStick API
 */

/**
 * Serial
 */

/*********************************************************************//**
* @brief
* @param[in]
**********************************************************************/
void Serial_Disable(void)
{
	// implement later
}
/*********************************************************************//**
* @brief
* @param[in]
**********************************************************************/
void Serial_CreateStream(void* Stream)
{
	// implement later
}

uint32_t Buttons_GetStatus(void)
{
	uint8_t ret = NO_BUTTON_PRESSED;

	if((GPIO_ReadValue(BUTTONS_BUTTON1_GPIO_PORT_NUM) & (1UL<<BUTTONS_BUTTON1_GPIO_BIT_NUM)) == 0x00)
	{
		ret |= BUTTONS_BUTTON1;
	}
	return ret;

}

#ifndef __LPC11UXX__ // TODO PDL lib for LPC1100 is not fully supported
void InitTimer(uint32_t frequency)
{
	TIM_TIMERCFG_Type TIM_ConfigStruct;
	TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
	uint32_t matchvalue;

	if(frequency > 100000)
		return;
	NVIC_DisableIRQ(TIMER0_IRQn);
	matchvalue = 1000000 / frequency;

	// Initialize timer 0, prescale count time of 1uS
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;

	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	//Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	//Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = FALSE;
	//Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct.ExtMatchOutputType =TIM_EXTMATCH_NOTHING;
	// Set Match value
	TIM_MatchConfigStruct.MatchValue   = matchvalue;

	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_Init(LPC_TIMER0, TIM_TIMER_MODE,&TIM_ConfigStruct);
	TIM_ConfigMatch(LPC_TIMER0,&TIM_MatchConfigStruct);

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
	/* Enable interrupt for timer 0 */
	NVIC_EnableIRQ(TIMER0_IRQn);
	// To start timer 0
	TIM_Cmd(LPC_TIMER0,ENABLE);
}

/*********************************************************************//**
* @brief
* @param[in]
**********************************************************************/
void DeInitTimer(void)
{
	NVIC_DisableIRQ(TIMER0_IRQn);
	TIM_Cmd(LPC_TIMER0,DISABLE);
	TIM_DeInit(LPC_TIMER0);
}
/************************************************************************/
/* LPCXpresso                                                                     */
/************************************************************************/
#ifdef __CODE_RED
// FIXME One of functions in this file have to be used by application, otherwise this bsp.c will be excluded --> so do __sys_write & __sys_readc
///////////////////////////////////////////////////////////////////////////////////////////////
// Function __write() / __sys_write
//
// Called by bottom level of printf routine within RedLib C library to write
// a character. With the default semihosting stub, this would write the character
// to the debugger console window . But this version writes
// the character to the LPC1768/RDB1768 UART.
int __sys_write (int iFileHandle, char *pcBuffer, int iLength)
{
	UART_Send(DEBUG_UART_PORT, (uint8_t*) pcBuffer, iLength, BLOCKING);
	return iLength;
}

// Function __readc() / __sys_readc
//
// Called by bottom level of scanf routine within RedLib C library to read
// a character. With the default semihosting stub, this would read the character
// from the debugger console window (which acts as stdin). But this version reads
// the character from the LPC1768/RDB1768 UART.
int __sys_readc (void)
{
	/*-- UARTGetChar --*/
	uint8_t tmp = 0;
	UART_Receive(DEBUG_UART_PORT, &tmp, 1, BLOCKING);
	return (int) tmp;
}
#endif
/************************************************************************/
/* KEIL                                                                     */
/************************************************************************/
#if defined(__CC_ARM)

#include <rt_misc.h>
#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

int fputc(int c, FILE *f) {
  return UART_Send(DEBUG_UART_PORT, (uint8_t*)&c, 1, BLOCKING);
}		   
int fgetc(FILE *f) {
	/*-- UARTGetChar --*/
	uint8_t tmp = 0;
	UART_Receive(DEBUG_UART_PORT, &tmp, 1, BLOCKING);
	return (int) tmp;
}			
int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}		   
void _ttywrch(int c) {
	UART_Send(DEBUG_UART_PORT, (uint8_t*)&c, 1, BLOCKING);
}				
void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}	 
#endif
/************************************************************************/
/* IAR                                                                     */
/************************************************************************/

#endif /*__LPC11UXX__*/
