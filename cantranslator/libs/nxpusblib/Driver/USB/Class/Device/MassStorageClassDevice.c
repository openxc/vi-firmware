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

#define  __INCLUDE_FROM_MS_DRIVER
#define  __INCLUDE_FROM_MASSSTORAGE_DEVICE_C
#include "MassStorageClassDevice.h"

void MS_Device_ProcessControlRequest(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	if (!(Endpoint_IsSETUPReceived()))
	  return;

	if (USB_ControlRequest.wIndex != MSInterfaceInfo->Config.InterfaceNumber)
	  return;

	switch (USB_ControlRequest.bRequest)
	{
		case MS_REQ_MassStorageReset:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				MSInterfaceInfo->State.IsMassStoreReset = true;
			}

			break;
		case MS_REQ_GetMaxLUN:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_Write_8(MSInterfaceInfo->Config.TotalLUNs - 1);
				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
	}
}

bool MS_Device_ConfigureEndpoints(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	memset(&MSInterfaceInfo->State, 0x00, sizeof(MSInterfaceInfo->State));

	for (uint8_t EndpointNum = 1; EndpointNum < ENDPOINT_TOTAL_ENDPOINTS; EndpointNum++)
	{
		uint16_t Size;
		uint8_t  Type;
		uint8_t  Direction;
		bool     DoubleBanked;

		if (EndpointNum == MSInterfaceInfo->Config.DataINEndpointNumber)
		{
			Size         = MSInterfaceInfo->Config.DataINEndpointSize;
			Direction    = ENDPOINT_DIR_IN;
			Type         = EP_TYPE_BULK;
			DoubleBanked = MSInterfaceInfo->Config.DataINEndpointDoubleBank;
		}
		else if (EndpointNum == MSInterfaceInfo->Config.DataOUTEndpointNumber)
		{
			Size         = MSInterfaceInfo->Config.DataOUTEndpointSize;
			Direction    = ENDPOINT_DIR_OUT;
			Type         = EP_TYPE_BULK;
			DoubleBanked = MSInterfaceInfo->Config.DataOUTEndpointDoubleBank;
		}
		else
		{
			continue;
		}

		if (!(Endpoint_ConfigureEndpoint(EndpointNum, Type, Direction, Size,
		                                 DoubleBanked ? ENDPOINT_BANK_DOUBLE : ENDPOINT_BANK_SINGLE)))
		{
			return false;
		}
	}

	return true;
}

void MS_Device_USBTask(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataOUTEndpointNumber);

	if (Endpoint_IsReadWriteAllowed())
	{
		if (MS_Device_ReadInCommandBlock(MSInterfaceInfo))
		{
			if (MSInterfaceInfo->State.CommandBlock.Flags & MS_COMMAND_DIR_DATA_IN)
			  Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataINEndpointNumber);

			bool SCSICommandResult = CALLBACK_MS_Device_SCSICommandReceived(MSInterfaceInfo);

			MSInterfaceInfo->State.CommandStatus.Status              = (SCSICommandResult) ? MS_SCSI_COMMAND_Pass : MS_SCSI_COMMAND_Fail;
			MSInterfaceInfo->State.CommandStatus.Signature           = CPU_TO_LE32(MS_CSW_SIGNATURE);
			MSInterfaceInfo->State.CommandStatus.Tag                 = MSInterfaceInfo->State.CommandBlock.Tag;
			MSInterfaceInfo->State.CommandStatus.DataTransferResidue = MSInterfaceInfo->State.CommandBlock.DataTransferLength;

			if (!(SCSICommandResult) && (le32_to_cpu(MSInterfaceInfo->State.CommandStatus.DataTransferResidue)))
			  Endpoint_StallTransaction();

			MS_Device_ReturnCommandStatus(MSInterfaceInfo);
		}
	}

	if (MSInterfaceInfo->State.IsMassStoreReset)
	{
		Endpoint_ResetEndpoint(MSInterfaceInfo->Config.DataOUTEndpointNumber);
		Endpoint_ResetEndpoint(MSInterfaceInfo->Config.DataINEndpointNumber);

		Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataOUTEndpointNumber);
		Endpoint_ClearStall();
		Endpoint_ResetDataToggle();
		Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataINEndpointNumber);
		Endpoint_ClearStall();
		Endpoint_ResetDataToggle();

		MSInterfaceInfo->State.IsMassStoreReset = false;
	}
}

static bool MS_Device_ReadInCommandBlock(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	uint16_t BytesProcessed;

	Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataOUTEndpointNumber);
	
	BytesProcessed = 0;
	while (Endpoint_Read_Stream_LE(&MSInterfaceInfo->State.CommandBlock,
	                               (sizeof(MS_CommandBlockWrapper_t) - 16), &BytesProcessed) ==
	                               ENDPOINT_RWSTREAM_IncompleteTransfer)
	{
		if (MSInterfaceInfo->State.IsMassStoreReset)
		  return false;
	}

	if ((MSInterfaceInfo->State.CommandBlock.Signature         != CPU_TO_LE32(MS_CBW_SIGNATURE))     ||
	    (MSInterfaceInfo->State.CommandBlock.LUN               >= MSInterfaceInfo->Config.TotalLUNs) ||
		(MSInterfaceInfo->State.CommandBlock.Flags              & 0x1F)                              ||
		(MSInterfaceInfo->State.CommandBlock.SCSICommandLength == 0)                                 ||
		(MSInterfaceInfo->State.CommandBlock.SCSICommandLength >  16))
	{
		Endpoint_StallTransaction();
		Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataINEndpointNumber);
		Endpoint_StallTransaction();

		return false;
	}

	BytesProcessed = 0;
	while (Endpoint_Read_Stream_LE(&MSInterfaceInfo->State.CommandBlock.SCSICommandData,
	                                MSInterfaceInfo->State.CommandBlock.SCSICommandLength, &BytesProcessed) ==
	                                ENDPOINT_RWSTREAM_IncompleteTransfer)
	{
		if (MSInterfaceInfo->State.IsMassStoreReset)
		  return false;
	}

	Endpoint_ClearOUT();

	return true;
}

static void MS_Device_ReturnCommandStatus(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataOUTEndpointNumber);

	while (Endpoint_IsStalled())
	{
		#if !defined(INTERRUPT_CONTROL_ENDPOINT)
		USB_USBTask();
		#endif

		if (MSInterfaceInfo->State.IsMassStoreReset)
		  return;
	}

	Endpoint_SelectEndpoint(MSInterfaceInfo->Config.DataINEndpointNumber);

	while (Endpoint_IsStalled())
	{
		#if !defined(INTERRUPT_CONTROL_ENDPOINT)
		USB_USBTask();
		#endif

		if (MSInterfaceInfo->State.IsMassStoreReset)
		  return;
	}

	uint16_t BytesProcessed = 0;
	while (Endpoint_Write_Stream_LE(&MSInterfaceInfo->State.CommandStatus,
	                                sizeof(MS_CommandStatusWrapper_t), &BytesProcessed) ==
	                                ENDPOINT_RWSTREAM_IncompleteTransfer)
	{
		if (MSInterfaceInfo->State.IsMassStoreReset)
		  return;
	}
	
	Endpoint_ClearIN();
}

#endif
