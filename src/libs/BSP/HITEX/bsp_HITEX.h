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



#ifndef __BSP_EA1800_H__
#define __BSP_EA1800_H__

#if defined(__LPC18XX__)
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
#endif

#define DEBUG_UART_PORT	((LPC_USARTn_Type*)LPC_USART0)
#define BUTTONS_BUTTON1_GPIO_PORT_NUM	6
#define BUTTONS_BUTTON1_GPIO_BIT_NUM	21

#define I2CDEV_PCA9502_ADDR		(0x9A>>1)
#define PCA9502_REG_IODIR		0x0A
#define PCA9502_REG_IOSTATE		0x0B
#define PCA9502_REG_IOINTENA	0x0C
#define PCA9502_REG_IOCONTROL	0x0E
#define PCA9502_REG_ADDR(x)		(((x) & 0x0F)<<3)

#endif
