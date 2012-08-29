/**********************************************************************
* $Id$		ipc_int.c			2012-03-16
*//**
* @file		ipc_int.c
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
#include <stdint.h>

#include "ipc_int.h"

/* definitions */
#ifdef IPC_MASTER

	#define IRQ_LOCK_KEY 
		
	#define _lockInts()		__set_BASEPRI((MASTER_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS) & 0xFF ))
	#define _unlockInts()	__set_BASEPRI(0)

	volatile intFlag_t intFlag;

#endif	

#ifdef IPC_SLAVE

	#define _lockInts() 	do {\
	wasMasked = __disable_irq();\
	} while(0)

	#define _unlockInts() do {\
	if(!wasMasked) 	__enable_irq();\
	} while(0)

	#define IRQ_LOCK_KEY	int	wasMasked;
	
	volatile intFlag_t intFlag;
					 
#endif	

static intCallback_t slaveIntCallback;
static intCallback_t masterIntCallback;	


/************************************************************************
 * LOCAL INTERRUPT FUNCTIONS
 ***********************************************************************/
void IPC_resetIntFlag(void) {

		/* on cortex M4/M3 use primask */
		/* on cortex M0 disable interrupts globally */
		IRQ_LOCK_KEY

		_lockInts();
		intFlag = NO_MSG;
		_unlockInts();
}

/************************************************************************
 * REMOTE INTERRUPT FUNCTIONS
 ***********************************************************************/

void IPC_sendInterrupt(void) {

	/* make sure all data transactions complete before next instruction is executed */
	__DSB();  	
							
	/* now trigger the remote processor */
	__sev();
}


/************************************************************************
 * FRAMEWORK INITIALIZATION FUNCTIONS
 ***********************************************************************/
/* initialize the slave interrupt ipc framework */
void IPC_slaveInitInterrupt(intCallback_t slaveCback) {	

	intFlag = NO_MSG;

	NVIC_DisableIRQ((IRQn_Type)MASTER_IRQn);

	MASTER_TXEV_QUIT();

	/* register the callback, executed within the interrupt context */
	slaveIntCallback = slaveCback;
	
	/* clear the interrupt */
	NVIC_ClearPendingIRQ((IRQn_Type)MASTER_IRQn);
			
	/* set the default priority for the interrupts */
	NVIC_SetPriority((IRQn_Type)MASTER_IRQn, SLAVE_INTERRUPT_PRIORITY);
				
	/* enable the interrupt */
	NVIC_EnableIRQ((IRQn_Type)MASTER_IRQn);
}
	
	
/* interrupt function on the slave side (master -> slave interrupt) */
void IPC_Master2Slave_IRQHandler() {		

	/* quit the interrupt */
	MASTER_TXEV_QUIT();

	/* call the interrupt callback function */
	(*slaveIntCallback)();

	intFlag = MSG_PENDING;
}


/* download a processor image to the SLAVE CPU */
void IPC_downloadSlaveImage(uint32_t slaveRomStart, const unsigned char slaveImage[], uint32_t imageSize)
{
  	uint32_t i;
	volatile uint8_t *pu8SRAM;

	IPC_haltSlave();

    /* Copy initialized sections into Slave code / constdata area */
	pu8SRAM = (uint8_t *) slaveRomStart;
	for (i = 0; i < imageSize; i++)
	{
		pu8SRAM[i] = slaveImage[i];
	 }

	/* Set Slave shadow pointer to begining of rom (where application is located) */
	SET_SLAVE_SHADOWREG(slaveRomStart);
}


/* take SLAVE processor out of reset */
void IPC_startSlave(void)
{
	volatile uint32_t u32REG, u32Val;
	
	/* Release Slave from reset, first read status */
	/* Notice, this is a read only register !!! */
	u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;
			
	/* If the M0 is being held in reset, release it */
	/* 1 = no reset, 0 = reset */
	while(!(u32REG & (1u << 24)))
	{
		u32Val = (~(u32REG) & (~(1 << 24)));
		LPC_RGU->RESET_CTRL1 = u32Val;
		u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;
	};
}

/* put the SLAVE processor back in reset */
void IPC_haltSlave(void) {

	volatile uint32_t u32REG, u32Val;
	
	/* Check if M0 is reset by reading status */
	u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;
			
	/* If the M0 has reset not asserted, halt it... */
	/* in u32REG, status register, 1 = no reset */
	while ((u32REG & (1u << 24)))
	{
		u32Val = ( (~u32REG) | (1 << 24));
		LPC_RGU->RESET_CTRL1 = u32Val;
		u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;			
	}
}


/* initialize the interrupt on the master side */
void IPC_masterInitInterrupt(intCallback_t masterCback)
{
	intFlag = NO_MSG;
   
	SLAVE_TXEV_QUIT();

	/* register the callback, executed within the interrupt context */
	masterIntCallback = masterCback;
	
	/* disable IRQ */
	NVIC_DisableIRQ((IRQn_Type)SLAVE_IRQn);	

	/* clear any pending interrupt */
	NVIC_ClearPendingIRQ((IRQn_Type)SLAVE_IRQn);

	/* set the default priority for the mbx interrupts */
	NVIC_SetPriority((IRQn_Type)SLAVE_IRQn, MASTER_INTERRUPT_PRIORITY);
			
	/* enable the interrupt */
	NVIC_EnableIRQ((IRQn_Type)SLAVE_IRQn);	
}

/* interrupt function on the master side (slave -> master interrupt) */
void IPC_Slave2Master_IRQHandler() {

	/* acknowledge the interrupt */
	SLAVE_TXEV_QUIT();

	/* call the interrupt callback function */
	(*masterIntCallback)();
	
	/* set the interrupt flag */	
	intFlag = MSG_PENDING;			
}




