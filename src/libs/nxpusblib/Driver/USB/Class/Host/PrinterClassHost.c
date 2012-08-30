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

#if defined(USB_CAN_BE_HOST)

#define  __INCLUDE_FROM_PRINTER_DRIVER
#define  __INCLUDE_FROM_PRINTER_HOST_C
#include "PrinterClassHost.h"

uint8_t PRNT_Host_ConfigurePipes(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                                 uint16_t ConfigDescriptorSize,
							     void* ConfigDescriptorData)
{
	USB_Descriptor_Endpoint_t*  DataINEndpoint   = NULL;
	USB_Descriptor_Endpoint_t*  DataOUTEndpoint  = NULL;
	USB_Descriptor_Interface_t* PrinterInterface = NULL;

	memset(&PRNTInterfaceInfo->State, 0x00, sizeof(PRNTInterfaceInfo->State));

	if (DESCRIPTOR_TYPE(ConfigDescriptorData) != DTYPE_Configuration)
	  return PRNT_ENUMERROR_InvalidConfigDescriptor;

	while (!(DataINEndpoint) || !(DataOUTEndpoint))
	{
		if (!(PrinterInterface) ||
		    USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
		                              DCOMP_PRNT_Host_NextPRNTInterfaceEndpoint) != DESCRIPTOR_SEARCH_COMP_Found)
		{
			if (USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
			                              DCOMP_PRNT_Host_NextPRNTInterface) != DESCRIPTOR_SEARCH_COMP_Found)
			{
				return PRNT_ENUMERROR_NoCompatibleInterfaceFound;
			}

			PrinterInterface = DESCRIPTOR_PCAST(ConfigDescriptorData, USB_Descriptor_Interface_t);

			DataINEndpoint  = NULL;
			DataOUTEndpoint = NULL;

			continue;
		}

		USB_Descriptor_Endpoint_t* EndpointData = DESCRIPTOR_PCAST(ConfigDescriptorData, USB_Descriptor_Endpoint_t);

		if ((EndpointData->EndpointAddress & ENDPOINT_DIR_MASK) == ENDPOINT_DIR_IN)
		  DataINEndpoint  = EndpointData;
		else
		  DataOUTEndpoint = EndpointData;
	}

	for (uint8_t PipeNum = 1; PipeNum < PIPE_TOTAL_PIPES; PipeNum++)
	{
		uint16_t Size;
		uint8_t  Type;
		uint8_t  Token;
		uint8_t  EndpointAddress;
		bool     DoubleBanked;

		if (PipeNum == PRNTInterfaceInfo->Config.DataINPipeNumber)
		{
			Size            = le16_to_cpu(DataINEndpoint->EndpointSize);
			EndpointAddress = DataINEndpoint->EndpointAddress;
			Token           = PIPE_TOKEN_IN;
			Type            = EP_TYPE_BULK;
			DoubleBanked    = PRNTInterfaceInfo->Config.DataINPipeDoubleBank;

			PRNTInterfaceInfo->State.DataINPipeSize = DataINEndpoint->EndpointSize;
		}
		else if (PipeNum == PRNTInterfaceInfo->Config.DataOUTPipeNumber)
		{
			Size            = le16_to_cpu(DataOUTEndpoint->EndpointSize);
			EndpointAddress = DataOUTEndpoint->EndpointAddress;
			Token           = PIPE_TOKEN_OUT;
			Type            = EP_TYPE_BULK;
			DoubleBanked    = PRNTInterfaceInfo->Config.DataOUTPipeDoubleBank;

			PRNTInterfaceInfo->State.DataOUTPipeSize = DataOUTEndpoint->EndpointSize;
		}
		else
		{
			continue;
		}

		if (!(Pipe_ConfigurePipe(PipeNum, Type, Token, EndpointAddress, Size,
		                         DoubleBanked ? PIPE_BANK_DOUBLE : PIPE_BANK_SINGLE)))
		{
			return PRNT_ENUMERROR_PipeConfigurationFailed;
		}
	}

	PRNTInterfaceInfo->State.InterfaceNumber  = PrinterInterface->InterfaceNumber;
	PRNTInterfaceInfo->State.AlternateSetting = PrinterInterface->AlternateSetting;
	PRNTInterfaceInfo->State.IsActive = true;

	return PRNT_ENUMERROR_NoError;
}

static uint8_t DCOMP_PRNT_Host_NextPRNTInterface(void* CurrentDescriptor)
{
	USB_Descriptor_Header_t* Header = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Header_t);

	if (Header->Type == DTYPE_Interface)
	{
		USB_Descriptor_Interface_t* Interface = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Interface_t);

		if ((Interface->Class    == PRNT_CSCP_PrinterClass)    &&
		    (Interface->SubClass == PRNT_CSCP_PrinterSubclass) &&
		    (Interface->Protocol == PRNT_CSCP_BidirectionalProtocol))
		{
			return DESCRIPTOR_SEARCH_Found;
		}
	}

	return DESCRIPTOR_SEARCH_NotFound;
}

static uint8_t DCOMP_PRNT_Host_NextPRNTInterfaceEndpoint(void* CurrentDescriptor)
{
	USB_Descriptor_Header_t* Header = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Header_t);

	if (Header->Type == DTYPE_Endpoint)
	{
		USB_Descriptor_Endpoint_t* Endpoint = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Endpoint_t);

		uint8_t EndpointType = (Endpoint->Attributes & EP_TYPE_MASK);

		if (EndpointType == EP_TYPE_BULK)
		  return DESCRIPTOR_SEARCH_Found;
	}
	else if (Header->Type == DTYPE_Interface)
	{
		return DESCRIPTOR_SEARCH_Fail;
	}

	return DESCRIPTOR_SEARCH_NotFound;
}

void PRNT_Host_USBTask(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return;

	#if !defined(NO_CLASS_DRIVER_AUTOFLUSH)
	PRNT_Host_Flush(PRNTInterfaceInfo);
	#endif
}

uint8_t PRNT_Host_SetBidirectionalMode(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	if (PRNTInterfaceInfo->State.AlternateSetting)
	{
		uint8_t ErrorCode;

		if ((ErrorCode = USB_Host_SetInterfaceAltSetting(PRNTInterfaceInfo->State.InterfaceNumber,
		                                                 PRNTInterfaceInfo->State.AlternateSetting)) != HOST_SENDCONTROL_Successful)
		{
			return ErrorCode;
		}
	}

	return HOST_SENDCONTROL_Successful;
}

uint8_t PRNT_Host_GetPortStatus(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                                uint8_t* const PortStatus)
{
	USB_ControlRequest = (USB_Request_Header_t)
		{
			.bmRequestType = (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE),
			.bRequest      = PRNT_REQ_GetPortStatus,
			.wValue        = 0,
			.wIndex        = PRNTInterfaceInfo->State.InterfaceNumber,
			.wLength       = sizeof(uint8_t),
		};

	Pipe_SelectPipe(PIPE_CONTROLPIPE);
	return USB_Host_SendControlRequest(PortStatus);
}

uint8_t PRNT_Host_SoftReset(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	USB_ControlRequest = (USB_Request_Header_t)
		{
			.bmRequestType = (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE),
			.bRequest      = PRNT_REQ_SoftReset,
			.wValue        = 0,
			.wIndex        = PRNTInterfaceInfo->State.InterfaceNumber,
			.wLength       = 0,
		};

	Pipe_SelectPipe(PIPE_CONTROLPIPE);
	return USB_Host_SendControlRequest(NULL);
}

uint8_t PRNT_Host_Flush(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return PIPE_READYWAIT_DeviceDisconnected;

	uint8_t ErrorCode;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataOUTPipeNumber);
	Pipe_Unfreeze();

	if (!(Pipe_BytesInPipe()))
	  return PIPE_READYWAIT_NoError;

	bool BankFull = !(Pipe_IsReadWriteAllowed());

	Pipe_ClearOUT();

	if (BankFull)
	{
		if ((ErrorCode = Pipe_WaitUntilReady()) != PIPE_READYWAIT_NoError)
		  return ErrorCode;

		Pipe_ClearOUT();
	}

	Pipe_Freeze();

	return PIPE_READYWAIT_NoError;
}

uint8_t PRNT_Host_SendByte(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                           const uint8_t Data)
{
	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return PIPE_READYWAIT_DeviceDisconnected;

	uint8_t ErrorCode;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataOUTPipeNumber);
	Pipe_Unfreeze();

	if (!(Pipe_IsReadWriteAllowed()))
	{
		Pipe_ClearOUT();

		if ((ErrorCode = Pipe_WaitUntilReady()) != PIPE_READYWAIT_NoError)
		  return ErrorCode;
	}

	Pipe_Write_8(Data);
	Pipe_Freeze();

	return PIPE_READYWAIT_NoError;
}

uint8_t PRNT_Host_SendString(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                             void* String)
{
	uint8_t ErrorCode;

	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return PIPE_RWSTREAM_DeviceDisconnected;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataOUTPipeNumber);
	Pipe_Unfreeze();

	if ((ErrorCode = Pipe_Write_Stream_LE(String, strlen(String), NULL)) != PIPE_RWSTREAM_NoError)
	  return ErrorCode;

	Pipe_ClearOUT();

	ErrorCode = Pipe_WaitUntilReady();

	Pipe_Freeze();

	return ErrorCode;
}

uint8_t PRNT_Host_SendData(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                           void* Buffer,
                           const uint16_t Length)
{
	uint8_t ErrorCode;

	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return PIPE_RWSTREAM_DeviceDisconnected;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataOUTPipeNumber);
	Pipe_Unfreeze();

	if ((ErrorCode = Pipe_Write_Stream_LE(Buffer, Length, NULL)) != PIPE_RWSTREAM_NoError)
	  return ErrorCode;

	Pipe_ClearOUT();

	ErrorCode = Pipe_WaitUntilReady();

	Pipe_Freeze();

	return ErrorCode;
}

uint16_t PRNT_Host_BytesReceived(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return 0;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataINPipeNumber);
	Pipe_Unfreeze();

	if (Pipe_IsINReceived())
	{
		if (!(Pipe_BytesInPipe()))
		{
			Pipe_ClearIN();
			Pipe_Freeze();
			return 0;
		}
		else
		{
			Pipe_Freeze();
			return Pipe_BytesInPipe();
		}
	}
	else
	{
		Pipe_Freeze();

		return 0;
	}
}

int16_t PRNT_Host_ReceiveByte(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo)
{
	if ((USB_HostState != HOST_STATE_Configured) || !(PRNTInterfaceInfo->State.IsActive))
	  return PIPE_RWSTREAM_DeviceDisconnected;

	int16_t ReceivedByte = -1;

	Pipe_SelectPipe(PRNTInterfaceInfo->Config.DataINPipeNumber);
	Pipe_Unfreeze();

	if (Pipe_IsINReceived())
	{
		if (Pipe_BytesInPipe())
		  ReceivedByte = Pipe_Read_8();

		if (!(Pipe_BytesInPipe()))
		  Pipe_ClearIN();
	}

	Pipe_Freeze();

	return ReceivedByte;
}

uint8_t PRNT_Host_GetDeviceID(USB_ClassInfo_PRNT_Host_t* const PRNTInterfaceInfo,
                              char* const DeviceIDString,
                              const uint16_t BufferSize)
{
	uint8_t  ErrorCode = HOST_SENDCONTROL_Successful;
	uint16_t DeviceIDStringLength = 0;

	USB_ControlRequest = (USB_Request_Header_t)
		{
			.bmRequestType = (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE),
			.bRequest      = PRNT_REQ_GetDeviceID,
			.wValue        = 0,
			.wIndex        = PRNTInterfaceInfo->State.InterfaceNumber,
			.wLength       = sizeof(DeviceIDStringLength),
		};

	Pipe_SelectPipe(PIPE_CONTROLPIPE);

	if ((ErrorCode = USB_Host_SendControlRequest(&DeviceIDStringLength)) != HOST_SENDCONTROL_Successful)
	  return ErrorCode;

	if (!(DeviceIDStringLength))
	{
		DeviceIDString[0] = 0x00;
		return HOST_SENDCONTROL_Successful;
	}

	DeviceIDStringLength = be16_to_cpu(DeviceIDStringLength);

	if (DeviceIDStringLength > BufferSize)
	  DeviceIDStringLength = BufferSize;

	USB_ControlRequest.wLength = DeviceIDStringLength;

	if ((ErrorCode = USB_Host_SendControlRequest(DeviceIDString)) != HOST_SENDCONTROL_Successful)
	  return ErrorCode;

	memmove(&DeviceIDString[0], &DeviceIDString[2], DeviceIDStringLength - 2);

	DeviceIDString[DeviceIDStringLength - 2] = 0x00;

	return HOST_SENDCONTROL_Successful;
}

#endif

