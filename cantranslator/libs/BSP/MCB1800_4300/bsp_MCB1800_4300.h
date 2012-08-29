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



#ifndef __BSP_MCB1800_4300_H__
#define __BSP_MCB1800_4300_H__

#if defined(__LPC43XX__)
#include "lpc43xx_uart.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_cgu.h"
#include "lpc43xx_gpio.h"
#include "lpc43xx_timer.h"
#include "lpc43xx_i2c.h"
#include "lpc43xx_gpdma.h"
#include "lpc43xx_i2s.h"
#include "lpc43xx_emc.h"
#elif defined(__LPC18XX__)
#include "lpc18xx_uart.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_gpio.h"
#include "lpc18xx_timer.h"
#include "lpc18xx_i2c.h"
#include "lpc18xx_gpdma.h"
#include "lpc18xx_i2s.h"
#include "lpc18xx_emc.h"
#endif

#define DEBUG_UART_PORT		(LPC_USARTn_Type *)LPC_USART3

#define BUTTONS_BUTTON1_GPIO_PORT_NUM	2
#define BUTTONS_BUTTON1_GPIO_BIT_NUM	0
#define LED1_GPIO_PORT_NUM				4
#define LED1_GPIO_BIT_NUM				14
#define LED2_GPIO_PORT_NUM				4
#define LED2_GPIO_BIT_NUM				13
#define LED3_GPIO_PORT_NUM				4
#define LED3_GPIO_BIT_NUM				12
#define LED4_GPIO_PORT_NUM				6
#define LED4_GPIO_BIT_NUM				28
#define JOYSTICK_UP_GPIO_PORT_NUM				6
#define JOYSTICK_UP_GPIO_BIT_NUM				10
#define JOYSTICK_DOWN_GPIO_PORT_NUM				6
#define JOYSTICK_DOWN_GPIO_BIT_NUM				11
#define JOYSTICK_LEFT_GPIO_PORT_NUM				6
#define JOYSTICK_LEFT_GPIO_BIT_NUM				12
#define JOYSTICK_RIGHT_GPIO_PORT_NUM			6
#define JOYSTICK_RIGHT_GPIO_BIT_NUM				13
#define JOYSTICK_PRESS_GPIO_PORT_NUM			6
#define JOYSTICK_PRESS_GPIO_BIT_NUM				8

#endif
