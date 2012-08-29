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



#ifndef __BSP_INTERNAL_H
#define __BSP_INTERNAL_H

#include "bsp.h"

/**
 * BOARD DEFINE
 */
#define BOARD_LPCXpressoBase_RevB       1
#define BOARD_MCB1000					2
#define BOARD_MCB1700					3
#define BOARD_MCB1800_4300				4
#define BOARD_HITEX_A4					5
#define BOARD_EAOEMBase_RevA			6
#define BOARD_ELE14_4350				7
#define BOARD_XPLORER					8

#if (BOARD == BOARD_HITEX_A4)
	#include "HITEX/bsp_HITEX.h"
#elif (BOARD == BOARD_LPCXpressoBase_RevB)
	#include "LPCXpressoBase_RevB/bsp_LPCXpressoBase_RevB.h"
#elif (BOARD == BOARD_MCB1700)
	#include "MCB1700/bsp_MCB1700.h"
#elif (BOARD == BOARD_MCB1000)
	#include "MCB1000/bsp_MCB1000.h"
#elif (BOARD == BOARD_XPLORER) || (BOARD == BOARD_ELE14_4350)
	#include "NGX/bsp_NGX.h"
#elif (BOARD == BOARD_EAOEMBase_RevA)
	#include "EAOEMBase_RevA/bsp_EAOEMBase_RevA.h"
#elif (BOARD == BOARD_MCB1800_4300)
	#include "MCB1800_4300/bsp_MCB1800_4300.h"
#else
	#error You must choose a board profile
#endif


#endif
