/*
* Copyright(C) NXP Semiconductors, 2011
* All rights reserved.
*
* Copyright (C) Dean Camera, 2011.
*
* LUFA Library is licensed from Dean Camera by NXP for NXP customers 
* for use with NXP's LPC microcontrollers.
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
 *  \brief Master include file for the library USB RNDIS Class driver.
 *
 *  Master include file for the library USB RNDIS Class driver, for both host and device modes, where available.
 *
 *  This file should be included in all user projects making use of this optional class driver, instead of
 *  including any headers in the USB/ClassDriver/Device, USB/ClassDriver/Host or USB/ClassDriver/Common subdirectories.
 */

/** \ingroup Group_USBClassDrivers
 *  \defgroup Group_USBClassRNDIS RNDIS (Networking) Class Driver
 *
 *  \section Sec_Dependencies Module Source Dependencies
 *  The following files must be built with any user project that uses this module:
 *    - nxpUSBlib/Drivers/USB/Class/Device/RNDIS.c <i>(Makefile source module name: NXPUSBLIB_SRC_USBCLASS)</i>
 *    - nxpUSBlib/Drivers/USB/Class/Host/RNDIS.c <i>(Makefile source module name: NXPUSBLIB_SRC_USBCLASS)</i>
 *
 *  \section Sec_ModDescription Module Description
 *  RNDIS Class Driver module. This module contains an internal implementation of the Microsoft USB RNDIS Networking
 *  Class, for both Device and Host USB modes. User applications can use this class driver instead of implementing the
 *  RNDIS class manually via the low-level nxpUSBlib APIs.
 *
 *  This module is designed to simplify the user code by exposing only the required interface needed to interface with
 *  Hosts using the USB RNDIS Class.
 *
 *  @{
 */

#ifndef _RNDIS_CLASS_H_
#define _RNDIS_CLASS_H_

	/* Macros: */
		#define __INCLUDE_FROM_USB_DRIVER
		#define __INCLUDE_FROM_RNDIS_DRIVER

	/* Includes: */
		#include "../Core/USBMode.h"

		#if defined(USB_CAN_BE_DEVICE)
			#include "Device/RNDISClassDevice.h"
		#endif

		#if defined(USB_CAN_BE_HOST)
			#include "Host/RNDISClassHost.h"
		#endif

#endif

/** @} */

