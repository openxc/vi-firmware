/**********************************************************************
* $Id$		ipc_buffer.c			2012-03-16
*//**
* @file		ipc_buffer.c
* @brief	LPC43xx Dual Core Mailbox buffer module
* @version	1.0
* @date		03. March. 2012
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2012, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
**********************************************************************/
#include <stdint.h>

#include "ipc_mbx.h"

/* place in a specific named section, for the linker scatter file */
#ifdef IPC_MASTER
Mbx Master_mbxTable[NUM_MASTER_MBX] __attribute__((section("Master_mbxTable"), used));
#endif

/* place in a specific named section, for the linker scatter file */ 
#ifdef IPC_SLAVE
Mbx Slave_mbxTable[NUM_SLAVE_MBX] __attribute__((section("Slave_mbxTable"), used));
#endif
	






