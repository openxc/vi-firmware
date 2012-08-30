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
* documentation is hereby granted, under NXP Semiconductors’ and its 
* licensor’s relevant copyrights in the software, without fee, provided that it 
* is used in conjunction with NXP Semiconductors microcontrollers.  This 
* copyright, permission, and disclaimer notice must appear in all copies of 
* this code.
*/



#ifndef __BSP_MCB1700_H__
#define __BSP_MCB1700_H__

#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_dac.h"

#define LPC_TIMER0			LPC_TIM0

#define DEBUG_UART_PORT		LPC_UART0
#define UART_FUNCNUM		1
#define UART_PORTNUM		0
#define UART_TX_PINNUM		2
#define UART_RX_PINNUM		3

#define USB_CONNECT_GPIO_PORT_NUM			2
#define USB_CONNECT_GPIO_BIT_NUM			9

#define BUTTONS_BUTTON1_GPIO_PORT_NUM			2
#define BUTTONS_BUTTON1_GPIO_BIT_NUM			10

#define JOYSTICK_UP_GPIO_PORT_NUM				1
#define JOYSTICK_UP_GPIO_BIT_NUM				24
#define JOYSTICK_DOWN_GPIO_PORT_NUM				1
#define JOYSTICK_DOWN_GPIO_BIT_NUM				26
#define JOYSTICK_LEFT_GPIO_PORT_NUM				1
#define JOYSTICK_LEFT_GPIO_BIT_NUM				23
#define JOYSTICK_RIGHT_GPIO_PORT_NUM			1
#define JOYSTICK_RIGHT_GPIO_BIT_NUM				25
#define JOYSTICK_PRESS_GPIO_PORT_NUM			1
#define JOYSTICK_PRESS_GPIO_BIT_NUM				20


#endif
