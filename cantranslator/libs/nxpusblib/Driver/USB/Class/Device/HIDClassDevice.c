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


#define  __INCLUDE_FROM_USB_DRIVER
#include "../../Core/USBMode.h"

#if defined(USB_CAN_BE_DEVICE)

#define  __INCLUDE_FROM_HID_DRIVER
#define  __INCLUDE_FROM_HID_DEVICE_C
#include "HIDClassDevice.h"

void HID_Device_ProcessControlRequest(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo)
{
	if (!(Endpoint_IsSETUPReceived()))
	  return;

	if (USB_ControlRequest.wIndex != HIDInterfaceInfo->Config.InterfaceNumber)
	  return;

	switch (USB_ControlRequest.bRequest)
	{
		case HID_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint16_t ReportSize = 0;
				uint8_t  ReportID   = (USB_ControlRequest.wValue & 0xFF);
				uint8_t  ReportType = (USB_ControlRequest.wValue >> 8) - 1;
				uint8_t  ReportData[HIDInterfaceInfo->Config.PrevReportINBufferSize];

				memset(ReportData, 0, sizeof(ReportData));

				CALLBACK_HID_Device_CreateHIDReport(HIDInterfaceInfo, &ReportID, ReportType, ReportData, &ReportSize);

				if (HIDInterfaceInfo->Config.PrevReportINBuffer != NULL)
				{
					memcpy(HIDInterfaceInfo->Config.PrevReportINBuffer, ReportData,
					       HIDInterfaceInfo->Config.PrevReportINBufferSize);
				}

				Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

				Endpoint_ClearSETUP();
				Endpoint_Write_Control_Stream_LE(ReportData, ReportSize);
				Endpoint_ClearOUT();
			}

			break;
		case HID_REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint16_t ReportSize = USB_ControlRequest.wLength;
				uint8_t  ReportID   = (USB_ControlRequest.wValue & 0xFF);
				uint8_t  ReportType = (USB_ControlRequest.wValue >> 8) - 1;
				uint8_t  ReportData[ReportSize];

				Endpoint_ClearSETUP();
				Endpoint_Read_Control_Stream_LE(ReportData, ReportSize);
				Endpoint_ClearIN();

				CALLBACK_HID_Device_ProcessHIDReport(HIDInterfaceInfo, ReportID, ReportType,
				                                     &ReportData[ReportID ? 1 : 0], ReportSize - (ReportID ? 1 : 0));
			}

			break;
		case HID_REQ_GetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_Write_8(HIDInterfaceInfo->State.UsingReportProtocol);
				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case HID_REQ_SetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				HIDInterfaceInfo->State.UsingReportProtocol = ((USB_ControlRequest.wValue & 0xFF) != 0x00);
			}

			break;
		case HID_REQ_SetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				HIDInterfaceInfo->State.IdleCount = ((USB_ControlRequest.wValue & 0xFF00) >> 6);
			}

			break;
		case HID_REQ_GetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_Write_8(HIDInterfaceInfo->State.IdleCount >> 2);
				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
	}
}

bool HID_Device_ConfigureEndpoints(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo)
{
	memset(&HIDInterfaceInfo->State, 0x00, sizeof(HIDInterfaceInfo->State));
	HIDInterfaceInfo->State.UsingReportProtocol = true;
	HIDInterfaceInfo->State.IdleCount           = 500;

	if (!(Endpoint_ConfigureEndpoint(HIDInterfaceInfo->Config.ReportINEndpointNumber, EP_TYPE_INTERRUPT,
									 ENDPOINT_DIR_IN, HIDInterfaceInfo->Config.ReportINEndpointSize,
									 HIDInterfaceInfo->Config.ReportINEndpointDoubleBank ? ENDPOINT_BANK_DOUBLE : ENDPOINT_BANK_SINGLE)))
	{
		return false;
	}

	return true;
}

void HID_Device_USBTask(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo)
{
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	Endpoint_SelectEndpoint(HIDInterfaceInfo->Config.ReportINEndpointNumber);

	if (Endpoint_IsReadWriteAllowed())
	{
		uint8_t  ReportINData[HIDInterfaceInfo->Config.PrevReportINBufferSize];
		uint8_t  ReportID     = 0;
		uint16_t ReportINSize = 0;

		memset(ReportINData, 0, sizeof(ReportINData));

		bool ForceSend         = CALLBACK_HID_Device_CreateHIDReport(HIDInterfaceInfo, &ReportID, HID_REPORT_ITEM_In,
		                                                             ReportINData, &ReportINSize);
		bool StatesChanged     = false;
		bool IdlePeriodElapsed = (HIDInterfaceInfo->State.IdleCount && !(HIDInterfaceInfo->State.IdleMSRemaining));

		if (HIDInterfaceInfo->Config.PrevReportINBuffer != NULL)
		{
			StatesChanged = (memcmp(ReportINData, HIDInterfaceInfo->Config.PrevReportINBuffer, ReportINSize) != 0);
			memcpy(HIDInterfaceInfo->Config.PrevReportINBuffer, ReportINData, HIDInterfaceInfo->Config.PrevReportINBufferSize);
		}

		if (ReportINSize && (ForceSend || StatesChanged || IdlePeriodElapsed))
		{
			HIDInterfaceInfo->State.IdleMSRemaining = HIDInterfaceInfo->State.IdleCount;

			Endpoint_SelectEndpoint(HIDInterfaceInfo->Config.ReportINEndpointNumber);

			if (ReportID)
			  Endpoint_Write_8(ReportID);

			Endpoint_Write_Stream_LE(ReportINData, ReportINSize, NULL);

			Endpoint_ClearIN();
		}
	}
}

#endif

