/**********************************************************************
* $Id$		ipc_queue.h			2012-03-16
*//**
* @file		ipc_queue.h
* @brief	LPC43xx Dual Core Queue definitions
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
#ifndef __IPC_QUEUE_H__
#define __IPC_QUEUE_H__



/* status definition of the queue */
typedef enum qStat_tag {

	QEMPTY = 0, /* nothing in the queue */
	QINSERT,	/* indicates a message has been successfully placed in the queue */
	QFULL, 		/* queue is full, message was not inserted */
	QVALID,		/* got a valid message which was extracted from the queue */
	QERROR

} qStat;


/**********************************************/
/* functions for queue management */
/**********************************************/

/* reset the messaging queue on the slave side */
qStat IPC_slaveFlushMsgQueue(void);

/* reset the command queue on the master side */
qStat IPC_masterFlushCmdQueue(void);


/************************************************/
/* types of ipc commands 						*/
/************************************************/

typedef enum cmd_tag {

	CMD_RD_ID = 0,
	CMD_WR_ID,
	NUM_CMD,
	INVALID_CMD

} cmd_t;   


typedef struct cmdToken_tag {
    
	union {

        uint32_t rawCmdWord;

        union { 			
			/* generic representation */
			struct {
            	uint16_t id;     	/* task Id */
            	uint16_t pload;  	/* argument */
        	} generic; 
						
			/* command */
			struct {
				uint16_t id;		
				uint16_t rW:4;			/* rw */
				uint16_t argument:12;	/* status information */
		   	} command; 

		} parsedCmd;

	} cmdClass;

}  cmdToken;

/* read token is just composed by command */
typedef cmdToken rdCmd;


/* write token includes a 32-bit parameter */
typedef struct ipcWriteToken_tag {

	cmdToken cmdToken;
	uint32_t param;

} wrCmd;


/* helper macros to create the appropriate header */
#define MAKE_READ_CMD_HEADER(rdCmd,id,arg) ( rdCmd.cmdClass.rawCmdWord = ((((arg & 0xFFF) << 4) & 0xFFF0) << 16) | (id & 0xFFFF) )
#define MAKE_WRITE_CMD_HEADER(wrCmd,id,arg) ( wrCmd.cmdToken.cmdClass.rawCmdWord = ((((arg & 0xFFF) << 4) | 1u) << 16) | (id & 0xFFFF) )

/* return the command type of the passed token */
cmd_t IPC_getCmdType(cmdToken* item);

/* write a command to the command queue */
qStat IPC_masterPushCmd(cmdToken* item);

/* signal to the slave processor there is something in the queue */
void IPC_cmdNotifySlave(void);


/* used to determine the greatest size for the commands */
typedef struct maxCmd_tag {

	union {
		rdCmd 	readCmd;
		wrCmd 	writeCmd;
	} maxCmd;

} maxCmd_t;

/* get a command from the command queue */
/* this removes the command from the queue */
/* upcasted to the greatest command type */
qStat IPC_slavePopCmd(maxCmd_t* item);

/************************************************/
/* types of ipc messages 						*/
/************************************************/

typedef enum msg_tag {

	MSG_SRV = 0,
	MSG_RD,
	MSG_RD_STS,
	MSG_WR_STS,
	NUM_MSG,
	INVALID_MSG

} msg_t;

typedef struct msgToken_tag {
    
	union {

        uint32_t rawMsgWord;

        union { 
			
			/* generic representation */
			struct {
            	uint16_t id;     	/* task Id */
            	uint16_t pload;  	/* argument */
        	} generic; 
			
			/* servicing answers */
			struct {
				uint16_t id;		
				uint16_t reserved:8;	/* reserved */
				uint16_t ss:8;			/* status information */

		   	} servicingAnswer; 
			
			/* read response */
			struct {
				uint16_t id;
				uint16_t ppp:12;  	/* argument id */
				uint16_t one:4;		/* set to one */
		   	} readResponse; 
			
			/* read status */
			struct {
				uint16_t id;
				uint16_t ppp:12;	/* argument id */
				uint16_t rStat:4;	/* value for read */
		   	} readStatus; 
			
			/* write status */
			struct {
				uint16_t id;
				uint16_t ppp:12;	/* argument id */
				uint16_t wStat:4;	/* write status */
		   	} writeStatus;  

		} parsedMsg;

    } msgClass;

} msgToken;


/* servicing messages are just composed by simple message */
typedef msgToken srvMsg;

/* read status messages are just composed by simple message */
typedef msgToken rdStsMsg;

/* write status messages are just composed by simple message */
typedef msgToken wrStsMsg;

/* read resposes include a 32-bit parameter */
typedef struct rdToken_tag {

	msgToken msgToken;
	uint32_t param;

} rdMsg;


/* helper macros to create appropriate headers */
#define MAKE_SRV_MSG_HEADER(srvMsg,id,sType) ( srvMsg.msgClass.rawMsgWord = ((((sType & 0xFF) << 8) & 0xFF00) << 16) | (id & 0xFFFF) )

#define MAKE_RD_MSG_HEADER(rdMsg,id,arg) ( rdMsg.msgToken.msgClass.rawMsgWord = ((((arg & 0xFFF) << 4) | 1u ) << 16) | (id & 0xFFFF) )

#define INVALID_ARG	(2u)
#define MAKE_RDSTS_MSG_HEADER(rdStsMsg,id,arg,failCode) ( rdStsMsg.msgClass.rawMsgWord = ((((arg & 0xFFF) << 4) | (failCode & 0xF)) << 16) | (id & 0xFFFF) )

#define WRITE_SUCCESSFUL	(5u)
#define WRITE_FAILED		(6u)

#define MAKE_WRSTS_MSG_HEADER(wrStsMsg,id,arg,response) ( wrStsMsg.msgClass.rawMsgWord = ((((arg & 0xFFF) << 4) | (0xF & response)) << 16) | (id & 0xFFFF) )




/* used to determine the greatest size for the messages */
typedef struct maxMsg_tag {

	union {
		srvMsg 		srvMsg;
		rdStsMsg 	rdStatMsg;
		wrStsMsg 	wrStsMsg;	  
		rdMsg		readMsg;
	} maxMsg;

} maxMsg_t;


/* return the message type of the passed token */
msg_t IPC_getMsgType(msgToken* item);

/* write a message to the message queue */
qStat IPC_slavePushMsg(msgToken* item);

/* get a message from the message queue */
qStat IPC_masterPopMsg(maxMsg_t* item);

/* signal to the slave processor there is something in the queue */
void IPC_msgNotifyMaster(void);


/**********************************************/
/* functions to initialize the framework */
/**********************************************/

/* download the SLAVE processor image */
void IPC_downloadSlaveImage(uint32_t slaveRomStart, const unsigned char slaveImage[], uint32_t imageSize);

/* take SLAVE processor out of reset */
void IPC_startSlave(void);

/* put the SLAVE processor back in reset */
void IPC_haltSlave(void);

/* initialize the Queue based ipc framework */
void IPC_masterInitQueue(cmdToken* cmdBuf, uint32_t cmdBufSize, msgToken* msgBuf, uint32_t msgBufSize);
void IPC_slaveInitQueue(void);

/**********************************************************/
/* definition of the flags used for application signaling */
/**********************************************************/
#define PENDING 	(1)
#define NO_TOKEN	(0)

typedef uint8_t flag_t;

/* helper functions to check the status of the queue */
uint8_t IPC_msgPending(void);
uint8_t IPC_cmdPending(void);

#include "ipc_bufdef.h"


#endif /* __IPC_QUEUE_H__ */
