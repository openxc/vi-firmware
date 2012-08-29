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

/** \file
 *
 *  Header file of HAL_LPC.c.
 *  This file contains common macros, APIs for upper layer (DCD, HCD) call,
 *  relating to init/deinit USB core, enable/disable USB interrupt...
 */

#ifndef __LPC_HAL_H__
#define __LPC_HAL_H__

/* Macros: */
/** These macros used to declare a variable in a defined section (ex: USB RAM section). */
#ifdef __CODE_RED
	#include <cr_section_macros.h>
#elif defined(__CC_ARM) // FIXME temporarily fix for Keil (work only for lpc17xx)
	#define __DATA(x)
	#define __BSS(x)
#endif
/* Chip Includes: */
#if defined(__LPC18XX__)||defined(__LPC43XX__)
	#include "LPC18XX/HAL_LPC18xx.h"
#elif defined(__LPC17XX__)||defined(__LPC177X_8X__)
	#include "LPC17XX/HAL_LPC17xx.h"
#elif defined(__LPC11UXX__)
	#include "LPC11UXX/HAL_LPC11Uxx.h"
#endif
/* Function Prototypes: */
/** This function is called by void USB_Init(void) to do the initialization for chip's USB core.
 *  Normally, this function will do the following:
 *     - Configure USB pins
 *     - Setup USB core clock
 *     - Call HAL_RESET to setup needed USB operating registers, set device address 0 if running device mode
 */
void HAL_USBInit(void);
/** This function usage is opposite to HAL_USBInit */
void HAL_USBDeInit(void);
/** This function used to enable USB interrupt */
void HAL_EnableUSBInterrupt(void);
/** This function usage is opposite to HAL_EnableUSBInterrupt */
void HAL_DisableUSBInterrupt(void);
/** This function is used in device mode to pull up resistor on USB pin D+
 *  Normally, this function is called when every setup or initial are done.
 */
void HAL_USBConnect (uint32_t con);
/* Selected USB Port Number */
extern uint8_t USBPortNum;
#endif /*__LPC_HAL_H__*/
/* --------------------------------- End Of File ------------------------------ */
