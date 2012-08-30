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



#ifndef __BSP_H__
#define __BSP_H__

#include "lpc_types.h"
#include <stdio.h>
#include <stdbool.h>
#include "TerminalCodes.h"	// FIXME temporarily

#define LEDS_LED1      		0x01
#define LEDS_LED2      		0x02
#define LEDS_LED3      		0x04
#define LEDS_LED4      		0x08
#define LEDS_NO_LEDS		0x00

#define BUTTONS_BUTTON1 	0x01

#define JOY_UP				0x01
#define JOY_DOWN			0x02
#define JOY_LEFT			0x04
#define JOY_RIGHT			0x08
#define JOY_PRESS			0x10
#define NO_BUTTON_PRESSED	0x00

void bsp_init(void);

void DeInitTimer(void);
void InitTimer(uint32_t frequency);
/**
 * Serial API
 */
void Serial_Disable(void);
void Serial_CreateStream(void* Stream);
void Serial_Init(const uint32_t BaudRate, const bool DoubleSpeed);
void Serial_Send(uint8_t* buffer, uint32_t size, TRANSFER_BLOCK_Type block_config);
uint32_t Serial_Revc(uint8_t* buffer, uint32_t size, TRANSFER_BLOCK_Type block_config);
/**
 * Button API
 */
void Buttons_Init(void);
uint32_t Buttons_GetStatus(void);


/**
 * LED API
 */
void LEDs_Init(void);
void LEDs_TurnOnLEDs(uint32_t LEDMask);
void LEDs_TurnOffLEDs(uint32_t LEDMask);
void LEDs_SetAllLEDs(uint32_t LEDMask);
void LEDs_ChangeLEDs(uint32_t LEDMask, uint32_t ActiveMask);
void LEDs_ToggleLEDs(uint32_t LEDMask);
uint32_t LEDs_GetLEDs(void);

/**
 * JoyStick API
 */
void Joystick_Init(void);
uint8_t Joystick_GetStatus(void);

/**
 * Audio API
 */
void Audio_Init(uint32_t samplefreq);
/** Reset Audio Buffer */
void Audio_Reset_Data_Buffer(void);
uint32_t Audio_Get_ISO_Buffer_Address(uint32_t last_packet_size);
/**
 * MassStorage API
 */
void MassStorage_Init(void);
uint32_t MassStorage_Cache_Flush(bool appcall);
void MassStorage_Write(uint32_t diskpos, void* buffer, uint32_t size);
void MassStorage_Read(uint32_t diskpos, void* buffer, uint32_t size);
bool MassStorage_Verify(void*dest, void* source, uint32_t size);
uint32_t MassStorage_GetCapacity(void);
bool MassStorage_FlushTimeOut(void);

void USB_Connect(void);

/**
 * Deprecated functions, used for compatible only
 */
#define sei()
#define puts_P(x)		puts(x)
#define PSTR(x)			x
#define printf_P		printf

#endif
