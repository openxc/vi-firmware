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
 *  Declare common macros, variables that can be shared between
 *  DCD (Endpoint_LPCxxxx, Device_LPCxxxx) and (Endpoint_LPC, EndpointStream_LPC)
 */

#ifndef __ENDPOINT_COMMON_H__
#define __ENDPOINT_COMMON_H__

/* Includes: */
#include "../HAL/HAL_LPC.h"

/* Macros: */
/** Size of share memory buffer that a device uses to communicate with host. */
#define USB_DATA_BUFFER_TEM_LENGTH		512

/* Global Variables: */
/** Share memory buffer. */
extern uint8_t usb_data_buffer[];
extern uint32_t usb_data_buffer_size;
/** Indexer rolling along the share memory buffer. Used to determine the offset
 *  of next read/write activities on share memory buffer or the total amount of data
 *  ready to be sent.
 */
extern uint32_t usb_data_buffer_index;
/** Store the current selected endpoint number, always the logical endpint number.
 *  Usually used as index of endpointhandle array.
 */
extern uint8_t endpointselected;
/** Array to store the physical endpoint number or the actual endpoint number that need
 *  to be configured for any USB transactions.
 */
extern uint8_t endpointhandle[];

#endif /* __ENDPOINT_COMMON_H__ */
