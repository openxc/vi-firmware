/*
* Copyright(C) NXP Semiconductors, 2012
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
 *  Header file for DataflashManager.c.
 */

#ifndef _DATARAM_H_
#define _DATARAM_H_
#include <stdint.h>

/* Defines: */
/** Start address and size of RAM area which used for disk image */
#if defined(__LPC18XX__) || defined(__LPC43XX__)
#define DATA_RAM_START_ADDRESS			0x20000000
#define DATA_RAM_PHYSICAL_SIZE			0x4000
#define DATA_RAM_VIRTUAL_SIZE			0x4000
#endif
#if defined(__LPC17XX__)
#define DATA_RAM_START_ADDRESS			0x20080000
#define DATA_RAM_PHYSICAL_SIZE			0x4000
#define DATA_RAM_VIRTUAL_SIZE			0x4000
#endif
#if defined(__LPC177X_8X__)
#define DATA_RAM_START_ADDRESS			0x20040000
#define DATA_RAM_PHYSICAL_SIZE			0x4000
#define DATA_RAM_VIRTUAL_SIZE			0x4000
#endif
#if defined(__LPC11UXX__)
#define DATA_RAM_START_ADDRESS			0x20080000
#define DATA_RAM_PHYSICAL_SIZE			0xa00
#define DATA_RAM_VIRTUAL_SIZE			0x4000 /* fake capacity to trick windows */
#endif


#endif

