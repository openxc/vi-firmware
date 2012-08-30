/**********************************************************************
* $Id$		ipc_msg_buffer.c			2012-03-16
*//**
* @file		ipc_msg_buffer.c
* @brief	LPC43xx Dual Core Queue Message buffer module
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

#include "ipc_queue.h"

/* block holding registers (pointers) and data for the messages */
msgBlock hostMsgBufferControl __attribute__ ((section ("msgBuffer_section")));


/* buffer space holding messages */
msgToken _hostMsgBufferData[MSG_BUF_LEN] __attribute__ ((section ("msgBuffer_section")));

