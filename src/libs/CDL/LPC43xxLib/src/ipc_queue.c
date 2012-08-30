/**********************************************************************
* $Id$		ipc_queue.c			2012-03-16
*//**
* @file		ipc_queue.c
* @brief	LPC43xx Dual Core Queue communication module
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
//#include "type.h"

#include "platform_init.h"
#include "ipc_queue.h"
#include "ipc_bufdef.h"

/************************************************************/
/* helper functions for the interrupt management			*/
/************************************************************/
#if (DEVICE==LPC43xx)

	#ifdef IPC_MASTER

	#define IRQ_LOCK_KEY  
	#define _lockInts()		__set_BASEPRI((MASTER_QUEUE_PRIORITY << (8 - __NVIC_PRIO_BITS) & 0xFF ))
	#define _unlockInts()	__set_BASEPRI(0)

	#endif

	#ifdef IPC_SLAVE

	#define IRQ_LOCK_KEY int wasMasked;

	#define _lockInts() 	do {\
	wasMasked = __disable_irq();\
	} while(0)
	
	#define _unlockInts() do {\
	if(!wasMasked) 	__enable_irq();\
	} while(0)

	/* do not remove this, it is only to make the code compile on slave side */
	const unsigned char LR0[1];

	#endif /* IPC SLAVE */

#endif	/* LPC4300 */


/****************************************/
/* local functions 						*/
/****************************************/

/* indicates if there is enough space in the queue for the command */
static qStat _peekCmdQueue(uint32_t msgSize);

/* returns values in "items" (1 = 1 token, as 32-bit word) */
static uint32_t _getCmdSize(cmd_t cmd);

/* returns values in items (1 = 1 token, as 32-bit word) */
static uint32_t _getMsgSize(msg_t msg);

/* indicates if there is enough space in the queue for the message */
static qStat _peekMsgQueue(uint32_t msgSize);

static qStat _peekCmdQueue(uint32_t itemSize);


/*************************************/
/* command buffer specific functions */
/*************************************/
cmdBlock* const cmdBlockAddress = (cmdBlock*) MASTER_CMD_BLOCK_START;

const cmdToken INVALID_CMD_TOKEN = { 0xFFFFFFFF };
const rdCmd INVALID_RD_CMD = { 0xFFFFFFFF };
const wrCmd INVALID_WR_CMD = { 0xFFFFFFFF, 0xFFFFFFFF };

/* place in a specific section to make it easy for the linker to drop it */
static volatile flag_t cmdPending __attribute__ ((section ("cmdPending_section"))) = NO_TOKEN;

uint8_t IPC_cmdPending(void)
{
	return(cmdPending);
}


/* determines the command type  */
cmd_t IPC_getCmdType(cmdToken* item)
{	
	uint16_t pload;
	cmd_t command;

	pload = item->cmdClass.parsedCmd.generic.pload;

	switch(pload & 0xF) {

	  case 0:
	  	command = CMD_RD_ID;
	  break;

	  case 1:
	  	command = CMD_WR_ID;
	  break;

	  default:
	  	command = INVALID_CMD;
	  break;
	};

	return (command);
}



static uint32_t _getCmdSize(cmd_t cmd)
{	
	uint32_t size;

	switch(cmd) {

	  case CMD_RD_ID:
	  	size = 1;
	  break;

	  case CMD_WR_ID:
	  	size = 2;
	  break;

	  case INVALID_CMD: 
	  default:
	  	size = 0;
	  break;
	};

	return (size);
}


/* returns if there is enough space in the queue for the command */
/* enough space means the actual message size plus one token */
/* this ensures the read and write pointers will never get equal when writing */
/* which eliminates ambiguity when read and write pointers are equal */
/* read pointer = write pointer means the queue is empty */
/* only the read pointer is allowed to get equal to the write pointer */
qStat _peekCmdQueue(uint32_t itemSize)
{
	cmdToken *targetAddress, *eA, *sA, *rP, *wP;

	rP = cmdBlockAddress->cmdRdPtr;
	wP = cmdBlockAddress->cmdWrPtr;

	if (rP == wP) 
	{
		/* quick check on emptiness */		
		return(QEMPTY);
	}
	
	eA = cmdBlockAddress->cmdQendAddress;
	sA = cmdBlockAddress->cmdQstartAddress;

	targetAddress =  wP + itemSize;	 
	
	if(targetAddress > eA) 
	{ 
		/* target address needs to be rolled back */
		targetAddress = sA + (targetAddress - eA) - 1;
		
		if(rP < wP)
		{
			/* check if we would override the read pointer */
			/* [---rP ---- wP ----] */
			if(targetAddress >= rP) return(QFULL);
		} 
		else { 
			/* [---wP ---- rP ----] */
			return (QFULL); /* if read pointer is ahead we override sice targetAddress rolls over */
		}; 	
	} 
	else { /* if we would not have to roll back */

		/* check if we would override the read pointer */
		/* [---wP ---- rP ----] */
		if(rP > wP) {
			if(targetAddress >= rP) return(QFULL);
		};
		
		/* this is ok, no override, no rollback */
		/* [---rP ---- wP ----] */
	};

	/* if tests are passed return INSERT */	
	return(QINSERT);
}

/* reset the command queue by clearing everything out */
qStat IPC_masterFlushCmdQueue(void)
{
	cmdToken *erase;

	cmdBlockAddress->cmdRdPtr = cmdBlockAddress->cmdQstartAddress;
	cmdBlockAddress->cmdWrPtr = cmdBlockAddress->cmdQstartAddress;

	for(erase=cmdBlockAddress->cmdQstartAddress; erase <= cmdBlockAddress->cmdQendAddress; erase++)
		*erase = INVALID_CMD_TOKEN;

	return(QEMPTY);
}

/* write a command to the command queue */
/* the new location of the write pointer, if there is enough space in the queue, */
/* will be the first free location in the queue */
/* the new write pointer must be able to point to the new free location */
qStat IPC_masterPushCmd(cmdToken* item)
{
	uint32_t cmdSize;
	cmd_t cmd;
	qStat status;
	cmdToken *wP, *eA, *sA;

	cmd = IPC_getCmdType(item);
	cmdSize = _getCmdSize(cmd);
	 
	/* check if there is enough space for the message */
	status = _peekCmdQueue(cmdSize);

	if(status == QFULL) return(status);

	wP = cmdBlockAddress->cmdWrPtr;
	eA = cmdBlockAddress->cmdQendAddress;
	sA = cmdBlockAddress->cmdQstartAddress;

	switch(cmd) {

		case CMD_RD_ID:
			
			*((rdCmd*) wP) =  *((rdCmd*)item);
			if(++wP > eA)	wP = sA;

		break;

		case CMD_WR_ID:

			*((rdCmd*) wP) =  *((rdCmd*)item);
			
			/* check for roll over, if there is space on the buffer or need to break */
			if((wP+1) > eA) {
				
				wP = sA; /* roll over the pointer */
				
				/* write the parameter directly as uint_32 */
				*((uint32_t*)wP) = ((wrCmd*) item)->param;
				
				if(++wP > eA)	wP = sA; /* update the write pointer */
			}
			else {
				
				/* write the parameter as for the message type */
				((wrCmd*)	wP)->param = ((wrCmd*) item)->param;
				
				if(++wP > eA)	wP = sA;	/* do it twice because of the size */
				if(++wP > eA)	wP = sA;
			}

		break;

		case INVALID_CMD:
		default:
			return(QERROR);
	
	};

	cmdBlockAddress->cmdWrPtr = wP;	  /* update write pointer */
	return(QINSERT);
}

void IPC_cmdNotifySlave(void)
{
		// make sure all data transactions complete before next instruction is executed
		__DSB();  	
							
		// now trigger the remote processor
		__sev();
}



/* get a command from the command queue */
/* after the command is retrieved, the queue contents are invalidated */
qStat IPC_slavePopCmd(maxCmd_t* item)
{
	IRQ_LOCK_KEY
	cmd_t cmd;
	cmdToken *rP, *wP, *eA, *sA;

	rP = cmdBlockAddress->cmdRdPtr;
	wP = cmdBlockAddress->cmdWrPtr;

	if(rP == wP) {
		_lockInts();
		cmdPending = NO_TOKEN;
		_unlockInts();
		return(QEMPTY);
	};

	cmd = IPC_getCmdType(rP);
	eA = cmdBlockAddress->cmdQendAddress;
	sA = cmdBlockAddress->cmdQstartAddress;

	switch(cmd) {

		case CMD_RD_ID:

			*((rdCmd*) item) =  *((rdCmd*)rP);
			*rP = INVALID_RD_CMD;  /* invalidate the data */

			if(++rP > eA)	rP = sA;

		break;

		case CMD_WR_ID:
			
			/* check for roll over */
			if((rP+1) > eA) {
				
				((cmdToken*)item)->cmdClass.rawCmdWord = rP->cmdClass.rawCmdWord;
			
				*rP = INVALID_CMD_TOKEN;

				rP = sA; /* roll over the pointer */
				
				/* read the parameter as uint_32 */
				((wrCmd*) item)->param = *((uint32_t*) rP);

				*rP = INVALID_CMD_TOKEN;

				if(++rP > eA)	rP = sA; /* update again the read pointer and roll over if needed */
			}
			else {
				
				/* can read the parameter as for the message type */
				*((wrCmd*) item) =  *((wrCmd*)rP);
	
				*((wrCmd*)rP) = INVALID_WR_CMD;
				
				if(++rP > eA)	rP = sA;	/* do it twice because of the size */				
				if(++rP > eA)	rP = sA;
			}

		break;

		case INVALID_CMD:

			*rP = INVALID_CMD_TOKEN;  /* just invalidate a single item to try to recover */
			if(++rP > eA)	rP = sA; /* update the read pointer by one and roll over if needed */
			
			cmdBlockAddress->cmdRdPtr = rP;
			

		/* fall through by purpose, let application know there was an error */
		default:
			return(QERROR);
		
		  
	};

	cmdBlockAddress->cmdRdPtr = rP;
	
	return(QVALID);
}




/* message buffer specific functions */


msgBlock* const msgBlockAddress = (msgBlock*) SLAVE_MSG_BLOCK_START;

const msgToken 	INVALID_MSG_TOKEN = { 0xFFFFFFFF };
const srvMsg 	INVALID_SRV_MSG = { 0xFFFFFFFF };
const rdMsg 	INVALID_RD_MSG = { 0xFFFFFFFF, 0xFFFFFFFF };
const rdStsMsg 	INVALID_RD_STS_MSG = { 0xFFFFFFFF };
const wrStsMsg 	INVALID_WR_STS_MSG = { 0xFFFFFFFF };


/*************************************/
/* message buffer specific functions */
/*************************************/
/* place in a specific section to make it easy for the linker to drop it */
static volatile flag_t msgPending __attribute__ ((section ("msgPending_section")))  = NO_TOKEN;


uint8_t IPC_msgPending(void)
{
	return(msgPending);
}

/* return the type of message */
msg_t IPC_getMsgType(msgToken* item)
{
	uint16_t pload;
	msg_t msg;

	pload = item->msgClass.parsedMsg.generic.pload;

	switch(pload & 0xF) {

	  case 0:
	  	msg = MSG_SRV;
	  break;

	  case 1:
	  	msg = MSG_RD;
	  break;

	  case 2:
	  	msg = MSG_RD_STS;
	  break;

	  case 5:
	  case 6:
	  	msg = MSG_WR_STS;
	  break;

	  default:
	  	msg = INVALID_MSG; 
	  break;
	
	};
	
	return (msg);	
}


/* returns values in items (1 = 1 token, as 32-bit word) */
static uint32_t _getMsgSize(msg_t message)
{	
	uint32_t size;

	switch(message) {

	  case MSG_SRV:
	  case MSG_RD_STS:
	  case MSG_WR_STS:
	  	size = 1;
	  break;

	  case MSG_RD:
	  	size = 2;
	  break;

	  case INVALID_MSG: 
	  default:
	  	size = 0;
	  break;
	};

	return (size);
}

/* returns if there is enough space in the queue for the message */
/* enough space means the actual message size plus one token */
/* this ensures the read and write pointers will never get equal when writing */
/* which eliminates ambiguity when read and write pointers are equal */
/* read pointer = write pointer means the queue is empty */
/* only the read pointer is allowed to get equal to the write pointer */
static qStat _peekMsgQueue(uint32_t itemSize)
{
	msgToken *targetAddress, *eA, *sA, *rP, *wP;;

	rP = msgBlockAddress->msgRdPtr;
	wP = msgBlockAddress->msgWrPtr;

	if (rP == wP) 
	{
		/* quick check on emptiness */		
		return(QEMPTY);
	}
	
	eA = msgBlockAddress->msgQendAddress;
	sA = msgBlockAddress->msgQstartAddress;

	targetAddress =  wP + itemSize;	 
	
	/* now verify if inserting the message leaves the write pointer at least one pointer before the read pointer */
	/* write and read pointer can be equal only when queue is empty */
	/* write pointer always has to point to next free spave */
	if(targetAddress > eA) 
	{ 
		/* target address needs to be rolled back */
		targetAddress = sA + (targetAddress - eA) - 1;
		
		if(rP < wP)
		{
			/* check if we would override the read pointer */
			/* [---rP ---- wP ----] */
			if(targetAddress >= rP) return(QFULL);
		} 
		else { 
			/* [---wP ---- rP ----] */
			return (QFULL); /* if read pointer is ahead we override sice targetAddress rolls over */
		}; 	
	} 
	else { /* if we would not have to roll back */

		/* check if we would override the read pointer */
		/* [---wP ---- rP ----] */
		if(rP > wP) {
			if(targetAddress >= rP) return(QFULL);
		};
		
		/* this is ok, no override, no rollback */
		/* [---rP ---- wP ----] */
	};

	/* if tests are passed return INSERT */	
	return(QINSERT);
}

/* reset the messaging queue */
qStat IPC_slaveFlushMsgQueue(void)
{
	msgToken *erase;

	msgBlockAddress->msgRdPtr = msgBlockAddress->msgQstartAddress;
	msgBlockAddress->msgWrPtr = msgBlockAddress->msgQstartAddress;

	for(erase=msgBlockAddress->msgQstartAddress; erase <= msgBlockAddress->msgQendAddress; erase++)
		*erase = INVALID_MSG_TOKEN;

	return(QEMPTY);
}

/* write a message to the message queue */
qStat IPC_slavePushMsg(msgToken* item)
{
	uint32_t msgSize;
	msg_t msg;
	qStat status;
	msgToken *wP, *eA, *sA;

	msg = IPC_getMsgType(item);
	msgSize = _getMsgSize(msg);
	 
	status = _peekMsgQueue(msgSize);

	if(status == QFULL) return(status);

	wP = msgBlockAddress->msgWrPtr;
	eA = msgBlockAddress->msgQendAddress;
	sA = msgBlockAddress->msgQstartAddress;

	switch(msg) {

		case MSG_SRV:  
			
			*((srvMsg*) wP) =  *((srvMsg*)item);
			if(++wP > eA)	wP = sA;

		break;

		case MSG_RD:	 

			*((rdMsg*) wP) =  *((rdMsg*)item);
			
			/* check for roll over, if there is space on the buffer or need to break */
			if((wP+1) > eA) {
				
				wP = sA; /* roll over the pointer */
				
				/* write the parameter directly as uint_32 */
				*((uint32_t*)wP) = ((rdMsg*) item)->param;
				
				if(++wP > eA)	wP = sA; /* update the write pointer */
			}
			else {
				
				/* write the parameter as for the message type */
				((rdMsg*)	wP)->param = ((rdMsg*) item)->param;
				
				if(++wP > eA)	wP = sA;	/* do it twice because of the size */
				if(++wP > eA)	wP = sA;
			}

		break;

		case MSG_RD_STS:	  

			*((rdStsMsg*) wP) =  *((rdStsMsg*)item);
			if(++wP > eA)	wP = sA;

		break;

		case MSG_WR_STS:	  

			*((wrStsMsg*) wP) =  *((wrStsMsg*)item);
			if(++wP > eA)	wP = sA;

		break;

		case INVALID_MSG:
		default:
			return(QERROR);
	
	};

	msgBlockAddress->msgWrPtr = wP;	  /* update write pointer */
	return(QINSERT);
}

void IPC_msgNotifyMaster(void)
{
		// make sure all data transactions complete before next instruction is executed
		__DSB();  	
							
		// now trigger the remote processor
		__sev();
}



/* get a message from the message queue */
/* after the command is retrieved, the queue contents are invalidated */
qStat IPC_masterPopMsg(maxMsg_t* item)
{
	IRQ_LOCK_KEY
	msg_t msg;
	msgToken *rP, *wP, *eA, *sA;

	rP = msgBlockAddress->msgRdPtr;
	wP = msgBlockAddress->msgWrPtr;

	if(rP == wP) {
		_lockInts();
		msgPending = NO_TOKEN;
		_unlockInts();
		return(QEMPTY);
	};

	msg = IPC_getMsgType(rP);
	eA = msgBlockAddress->msgQendAddress;
	sA = msgBlockAddress->msgQstartAddress;

	switch(msg) {
		
		case MSG_SRV:

			*((srvMsg*) item) =  *((srvMsg*)rP);
			*rP = INVALID_SRV_MSG;  /* invalidate the data */

			if(++rP > eA)	rP = sA;

		break;

		case MSG_RD:
			
			/* check for roll over */
			if((rP+1) > eA) {
				
				((msgToken*)item)->msgClass.rawMsgWord = rP->msgClass.rawMsgWord;
		
				*rP = INVALID_MSG_TOKEN;

				rP = sA; /* roll over the pointer */
				
				/* read the parameter as uint_32 */
				((rdMsg*) item)->param = *((uint32_t*)rP);

				*rP = INVALID_MSG_TOKEN;

				if(++rP > eA)	rP = sA; /* update again the read pointer and roll over if needed */
			}
			else {
				
				/* can read the parameter as for the message type */
				*((rdMsg*) item) =  *((rdMsg*)rP);
	
				*((rdMsg*)rP) = INVALID_RD_MSG;
				
				if(++rP > eA)	rP = sA;	/* do it twice because of the size */				
				if(++rP > eA)	rP = sA;
			}

		break;

		case MSG_RD_STS:

			*((rdStsMsg*) item) =  *((rdStsMsg*)rP);

			*rP = INVALID_RD_STS_MSG;  /* invalidate the data */

			if(++rP > eA)	rP = sA;

		break;

		case MSG_WR_STS:

			*((wrStsMsg*) item) =  *((wrStsMsg*)rP);

			*rP = INVALID_WR_STS_MSG;  /* invalidate the data */

			if(++rP > eA)	rP = sA;

		break;

		case INVALID_MSG:

			*rP = INVALID_MSG_TOKEN;  /* just invalidate a single item to try to recover */
			if(++rP > eA)	rP = sA; /* update the read pointer by one and roll over if needed */
			
			msgBlockAddress->msgRdPtr = rP;
			

		/* fall through by purpose, let application know there was an error */
		default:
			return(QERROR);
		
		  
	};

	msgBlockAddress->msgRdPtr = rP;
	
	return(QVALID);
}





/***************************************
 * slave side initialization functions *
 ***************************************/
/* initialize the Queue ipc framework */
void IPC_slaveInitQueue(void)
{	
	MASTER_TXEV_QUIT();

	NVIC_DisableIRQ((IRQn_Type)MASTER_IRQn);

	// clear the interrupt
	NVIC_ClearPendingIRQ((IRQn_Type)MASTER_IRQn);
			
	// set the default priority for the interrupts
	NVIC_SetPriority((IRQn_Type)MASTER_IRQn, SLAVE_QUEUE_PRIORITY);
					
	cmdPending = NO_TOKEN;
		
	// enable the interrupt
	NVIC_EnableIRQ((IRQn_Type)MASTER_IRQn);
}
	
	
/* interrupt function for the Slave queue */
void IPC_Master2Slave_IRQHandler(void)
{		
	
	MASTER_TXEV_QUIT();

	cmdPending = PENDING;
}




/*******************************************
 * master side initialization functions		
 ******************************************/

/* download a processor image to the SLAVE CPU */
void IPC_downloadSlaveImage(uint32_t slaveRomStart, const unsigned char slaveImage[], uint32_t imageSize)
{
  	uint32_t i;
	volatile uint8_t *pu8SRAM;

	IPC_haltSlave();

    //Copy application into Slave ROM 
	pu8SRAM = (uint8_t *) slaveRomStart;
	for (i = 0; i < imageSize; i++)
	{
		pu8SRAM[i] = slaveImage[i];
	 }

	// Set Slave shadow pointer to begining of rom (where application is located) 
	SET_SLAVE_SHADOWREG(slaveRomStart);
}

/* take SLAVE processor out of reset */
void IPC_startSlave(void)
{
	volatile uint32_t u32REG, u32Val;
	
	// Release Slave from reset, first read status 
	u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;
			
	// If the M0 is being held in reset, release it... 
	// 1 = no reset, 0 = reset
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
	
	// Check if M0 is reset by reading status
	u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;
			
	// If the M0 has reset not asserted, halt it... 
	// in u32REG, status register, 1 = no reset
	while ((u32REG & (1u << 24)))
	{
		u32Val = ( (~u32REG) | (1 << 24));
		LPC_RGU->RESET_CTRL1 = u32Val;
		u32REG = LPC_RGU->RESET_ACTIVE_STATUS1;			
	}
}


void IPC_masterInitQueue(cmdToken* cmdBuf, uint32_t cmdBufSize, msgToken* msgBuf, uint32_t msgBufSize)
{	
	uint32_t i;
					 
	cmdBlockAddress->cmdQstartAddress = cmdBuf;
	cmdBlockAddress->cmdQendAddress = cmdBuf + cmdBufSize;	// end address includes +1 item
	cmdBlockAddress->cmdRdPtr =  cmdBuf;
	cmdBlockAddress->cmdWrPtr = cmdBuf;

	for(i=0; i < cmdBufSize; i++, cmdBuf++) *cmdBuf = INVALID_CMD_TOKEN;

	msgBlockAddress->msgQstartAddress = msgBuf;
	msgBlockAddress->msgQendAddress =  msgBuf + msgBufSize;	 // end address includes +1 item
	msgBlockAddress->msgRdPtr =  msgBuf;
	msgBlockAddress->msgWrPtr =  msgBuf;

	for(i=0; i < msgBufSize; i++, msgBuf++) *msgBuf = INVALID_MSG_TOKEN;

	SLAVE_TXEV_QUIT();

	// disable IRQ
	NVIC_DisableIRQ((IRQn_Type)SLAVE_IRQn);	

	// clear any pending interrupt
	NVIC_ClearPendingIRQ((IRQn_Type)SLAVE_IRQn);
			
	// clear the flag
	msgPending = NO_TOKEN;	

	// set the default priority for the ipc interrupts
	NVIC_SetPriority((IRQn_Type)SLAVE_IRQn, MASTER_QUEUE_PRIORITY);
			
	// enable the interrupt
	NVIC_EnableIRQ((IRQn_Type)SLAVE_IRQn);
		
}


/* interrupt function setting the flags for the application */
/* interrupt to master from slave */
void IPC_Slave2Master_IRQHandler()
{	
	SLAVE_TXEV_QUIT();

	msgPending = PENDING;
}




