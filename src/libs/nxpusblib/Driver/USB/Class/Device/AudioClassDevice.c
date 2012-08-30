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

#define  __INCLUDE_FROM_AUDIO_DRIVER
#define  __INCLUDE_FROM_AUDIO_DEVICE_C
#include "AudioClassDevice.h"

void Audio_Device_ProcessControlRequest(USB_ClassInfo_Audio_Device_t* const AudioInterfaceInfo)
{
	if (!(Endpoint_IsSETUPReceived()))
	  return;

	if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_INTERFACE)
	{
		if (USB_ControlRequest.wIndex != AudioInterfaceInfo->Config.StreamingInterfaceNumber)
		  return;
	}
	else if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_ENDPOINT)
	{
		bool EndpointFilterMatch = false;
	
		EndpointFilterMatch |= (AudioInterfaceInfo->Config.DataINEndpointNumber &&
		                        ((uint8_t)USB_ControlRequest.wIndex == (ENDPOINT_DIR_IN  | AudioInterfaceInfo->Config.DataINEndpointNumber)));
	
		EndpointFilterMatch |= (AudioInterfaceInfo->Config.DataOUTEndpointNumber &&
		                        ((uint8_t)USB_ControlRequest.wIndex == (ENDPOINT_DIR_OUT | AudioInterfaceInfo->Config.DataOUTEndpointNumber)));

		if (!(EndpointFilterMatch))
		  return;
	}	

	switch (USB_ControlRequest.bRequest)
	{
		case REQ_SetInterface:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				AudioInterfaceInfo->State.InterfaceEnabled = ((USB_ControlRequest.wValue & 0xFF) != 0);
				EVENT_Audio_Device_StreamStartStop(AudioInterfaceInfo);
			}

			break;
		case AUDIO_REQ_GetStatus:
			if ((USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) ||
			    (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_ENDPOINT)))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();
			}

			break;
		case AUDIO_REQ_SetCurrent:
		case AUDIO_REQ_SetMinimum:
		case AUDIO_REQ_SetMaximum:
		case AUDIO_REQ_SetResolution:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_ENDPOINT))
			{
				uint8_t EndpointProperty = USB_ControlRequest.bRequest;
				uint8_t EndpointAddress  = (uint8_t)USB_ControlRequest.wIndex;
				uint8_t EndpointControl  = (USB_ControlRequest.wValue >> 8);
			
				if (CALLBACK_Audio_Device_GetSetEndpointProperty(AudioInterfaceInfo, EndpointProperty, EndpointAddress,
				                                                 EndpointControl, NULL, NULL))
				{
					uint16_t ValueLength = USB_ControlRequest.wLength;
					uint8_t  Value[ValueLength];
					
					Endpoint_ClearSETUP();
					Endpoint_Read_Control_Stream_LE(Value, ValueLength);
					Endpoint_ClearIN();					

					CALLBACK_Audio_Device_GetSetEndpointProperty(AudioInterfaceInfo, EndpointProperty, EndpointAddress,
					                                             EndpointControl, &ValueLength, Value);
				}				
			}

			break;
		case AUDIO_REQ_GetCurrent:
		case AUDIO_REQ_GetMinimum:
		case AUDIO_REQ_GetMaximum:
		case AUDIO_REQ_GetResolution:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT))
			{
				uint8_t  EndpointProperty = USB_ControlRequest.bRequest;
				uint8_t  EndpointAddress  = (uint8_t)USB_ControlRequest.wIndex;
				uint8_t  EndpointControl  = (USB_ControlRequest.wValue >> 8);
				uint16_t ValueLength      = USB_ControlRequest.wLength;
				uint8_t  Value[ValueLength];

				if (CALLBACK_Audio_Device_GetSetEndpointProperty(AudioInterfaceInfo, EndpointProperty, EndpointAddress,
				                                                 EndpointControl, &ValueLength, Value))
				{
					Endpoint_ClearSETUP();
					Endpoint_Write_Control_Stream_LE(Value, ValueLength);
					Endpoint_ClearOUT();					
				}
			}

			break;
	}
}

bool Audio_Device_ConfigureEndpoints(USB_ClassInfo_Audio_Device_t* const AudioInterfaceInfo)
{
	memset(&AudioInterfaceInfo->State, 0x00, sizeof(AudioInterfaceInfo->State));

	for (uint8_t EndpointNum = 1; EndpointNum < ENDPOINT_TOTAL_ENDPOINTS; EndpointNum++)
	{
		uint16_t Size;
		uint8_t  Type;
		uint8_t  Direction;
		bool     DoubleBanked;

		if (EndpointNum == AudioInterfaceInfo->Config.DataINEndpointNumber)
		{
			Size         = AudioInterfaceInfo->Config.DataINEndpointSize;
			Direction    = ENDPOINT_DIR_IN;
			Type         = EP_TYPE_ISOCHRONOUS;
			DoubleBanked = true;
		}
		else if (EndpointNum == AudioInterfaceInfo->Config.DataOUTEndpointNumber)
		{
			Size         = AudioInterfaceInfo->Config.DataOUTEndpointSize;
			Direction    = ENDPOINT_DIR_OUT;
			Type         = EP_TYPE_ISOCHRONOUS;
			DoubleBanked = true;
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

void Audio_Device_Event_Stub(void)
{

}

#endif
