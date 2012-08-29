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



#ifndef __BSP_NGX_H__
#define __BSP_NGX_H__

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

#if (BOARD == BOARD_XPLORER4330)||(BOARD == BOARD_XPLORER1830)
#define DEBUG_UART_PORT	((LPC_USARTn_Type*)LPC_UART1) 		// Xplorer
#else
#define DEBUG_UART_PORT	((LPC_USARTn_Type*)LPC_USART3) 		// Farnell
#endif

#define BUTTONS_BUTTON1_GPIO_PORT_NUM	0
#define BUTTONS_BUTTON1_GPIO_BIT_NUM	7
#define LED1_GPIO_PORT_NUM				1
#define LED1_GPIO_BIT_NUM				11
#define LED2_GPIO_PORT_NUM				1
#define LED2_GPIO_BIT_NUM				12

#ifdef __CODE_RED
	#include <cr_section_macros.h>
#elif defined(__CC_ARM) // FIXME temporarily fix for Keil (work only for lpc17xx)
	#define __DATA(x)	__attribute__((section("usbram")))
#endif
#define USBRAM_SECTION RAM2


#endif
