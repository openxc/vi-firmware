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


#ifndef __HAL_LPC18XX_H__
#define __HAL_LPC18XX_H__

#if defined(__LPC18XX__)
#include "LPC18xx.h"
#endif
#if defined(__LPC43XX__)
#include "LPC43xx.h"
#endif
#include "lpc_types.h"

#define  __INCLUDE_FROM_USB_DRIVER
#include "../../../USBMode.h"

#define USBRAM_SECTION	RAM4

/* bit defines for DEVICEADDR register */
#define USBDEV_ADDR_AD	(1<<24)
#define USBDEV_ADDR(n)	(((n) & 0x7F)<<25)

#define USB_REG(HostID)			((HostID) ? ((LPC_USB0_Type*) LPC_USB1_BASE) : LPC_USB0)

extern void HcdIrqHandler(uint8_t HostID);
extern void DcdIrqHandler (uint8_t HostID);
void HAL_Reset (void);

#endif // __HAL_LPC18XX_H__
