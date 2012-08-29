/*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
*         LUFA Library
* Copyright (C) Dean Camera, 2011.
*
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
*
* Permission to use, copy, modify, and distribute this software
* and its documentation for any purpose is hereby granted without
* fee, provided that it is used in conjunction with NXP Semiconductors
* microcontrollers.  This copyright, permission, and disclaimer notice
* must appear in all copies of this code.
*/

/** \file
 *
 *  Header file for USBMemory.c.
 */

#ifndef __USBMEMORY_H__
#define __USBMEMORY_H__

/* Includes: */
#include "lpc_types.h"
#include "../../../Common/Common.h"

/* Function Prototypes: */
void USB_Memory_Init(uint32_t Memory_Pool_Size);
uint8_t* USB_Memory_Alloc(uint32_t size);
void USB_Memory_Free(uint8_t *ptr);

#endif /* __USBMEMORY_H__ */
