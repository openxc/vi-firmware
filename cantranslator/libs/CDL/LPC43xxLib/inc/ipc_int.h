/**********************************************************************
* $Id$		ipc_int.h			2012-03-16
*//**
* @file		ipc_int.h
* @brief	LPC43xx Dual Core Interrupt communication module
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
#ifndef __IPC_H__
#define __IPC_H__

#include "platform_init.h"

/* definition of interrupt status */
#define MSG_PENDING (1)
#define NO_MSG		(2)

typedef uint8_t intFlag_t;

#define LOOP_INSTRUCTION	0xE7FEE7FE

/**********************************************/
/* functions for interrupt handling */
/**********************************************/
/**********************************************/
/* callback definition */
/**********************************************/
	
typedef void (*intCallback_t) (void);

/**********************************************/
/* local interrupt functions */
/**********************************************/

/* reset the interrupt flag */
/* shortly disables interrupts */
void IPC_resetIntFlag(void);

/**********************************************/
/* remote mailbox functions 				  */
/**********************************************/

/* send a message to a specific mailbox */
/* configures the mailbox as process, triggers an interrupt to the remote cpu */
void IPC_sendInterrupt(void);

extern volatile intFlag_t intFlag;

#include "master_interrupt_callback.h"
#include "slave_interrupt_callback.h"

/**********************************************/
/* functions to initialize the framework */
/**********************************************/

/* download a processor image to the slave CPU */
void IPC_downloadSlaveImage(uint32_t SlaveRomStart, const unsigned char slaveImage[], uint32_t imageSize);

/* take processor out of reset */
void IPC_startSlave(void);

/* put the processor back in reset */
void IPC_haltSlave(void);

/* initialize the interrupt ipc framework */
void IPC_slaveInitInterrupt(intCallback_t slaveCback);
void IPC_masterInitInterrupt(intCallback_t masterCback);


#endif /* __IPC_H__ */
