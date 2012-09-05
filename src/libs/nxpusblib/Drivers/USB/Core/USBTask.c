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


#define  __INCLUDE_FROM_USBTASK_C
#define  __INCLUDE_FROM_USB_DRIVER
#include "USBTask.h"

volatile bool        USB_IsInitialized;
USB_Request_Header_t USB_ControlRequest __DATA(USBRAM_SECTION);

#if defined(USB_CAN_BE_HOST) && !defined(HOST_STATE_AS_GPIOR)
volatile uint8_t     USB_HostState;
#endif

#if defined(USB_CAN_BE_DEVICE) && !defined(DEVICE_STATE_AS_GPIOR)
volatile uint8_t     USB_DeviceState;
#endif

void USB_USBTask(void)
{
	#if defined(USB_HOST_ONLY)
		USB_HostTask();
	#elif defined(USB_DEVICE_ONLY)
		USB_DeviceTask();
	#else
		if (USB_CurrentMode == USB_MODE_Device)
		  USB_DeviceTask();
		else if (USB_CurrentMode == USB_MODE_Host)
		  USB_HostTask();
	#endif
}

#if defined(USB_CAN_BE_DEVICE)
static void USB_DeviceTask(void)
{
	if (USB_DeviceState != DEVICE_STATE_Unattached)
	{
		uint8_t PrevEndpoint = Endpoint_GetCurrentEndpoint();

		Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

		if (Endpoint_IsSETUPReceived())
		  USB_Device_ProcessControlRequest();

		Endpoint_SelectEndpoint(PrevEndpoint);
	}
}
#endif

#if defined(USB_CAN_BE_HOST)
static void USB_HostTask(void)
{
	uint8_t PrevPipe = Pipe_GetCurrentPipe();

	Pipe_SelectPipe(PIPE_CONTROLPIPE);

	USB_Host_ProcessNextHostState();

	Pipe_SelectPipe(PrevPipe);
}
#endif

