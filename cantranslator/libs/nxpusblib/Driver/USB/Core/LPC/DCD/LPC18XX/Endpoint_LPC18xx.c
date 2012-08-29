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

#define  __INCLUDE_FROM_USB_DRIVER
#include "../../../USBMode.h"

#if (defined(__LPC18XX__)||defined(__LPC43XX__)) && defined(USB_CAN_BE_DEVICE)
#include "../../../Endpoint.h"
#include <string.h>

DeviceQueueHead dQueueHead[USED_PHYSICAL_ENDPOINTS] ATTR_ALIGNED(2048) __DATA(USBRAM_SECTION);
DeviceTransferDescriptor dTransferDescriptor[USED_PHYSICAL_ENDPOINTS] ATTR_ALIGNED(32) __DATA(USBRAM_SECTION);
uint8_t iso_buffer[512] ATTR_ALIGNED(4);

uint32_t CALLBACK_HAL_GetISOBufferAddress(const uint32_t EPNum, uint32_t* last_packet_size) ATTR_WEAK ATTR_ALIAS(Dummy_EPGetISOAddress);

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_Reset (void)
{
  uint32_t i;

  /* disable all EPs */
  USB_REG(USBPortNum)->ENDPTCTRL0 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
  USB_REG(USBPortNum)->ENDPTCTRL1 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
  USB_REG(USBPortNum)->ENDPTCTRL2 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
  USB_REG(USBPortNum)->ENDPTCTRL3 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
  if(USB_REG(USBPortNum) == LPC_USB0)
  {
	  USB_REG(USBPortNum)->ENDPTCTRL4 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
	  USB_REG(USBPortNum)->ENDPTCTRL5 &= ~(ENDPTCTRL_RxEnable | ENDPTCTRL_TxEnable);
  }

  /* Clear all pending interrupts */
  USB_REG(USBPortNum)->ENDPTNAK		= 0xFFFFFFFF;
  USB_REG(USBPortNum)->ENDPTNAKEN		= 0;
  USB_REG(USBPortNum)->USBSTS_D		= 0xFFFFFFFF;
  USB_REG(USBPortNum)->ENDPTSETUPSTAT	= USB_REG(USBPortNum)->ENDPTSETUPSTAT;
  USB_REG(USBPortNum)->ENDPTCOMPLETE	= USB_REG(USBPortNum)->ENDPTCOMPLETE;
  while (USB_REG(USBPortNum)->ENDPTPRIME);                  /* Wait until all bits are 0 */
  USB_REG(USBPortNum)->ENDPTFLUSH = 0xFFFFFFFF;
  while (USB_REG(USBPortNum)->ENDPTFLUSH); /* Wait until all bits are 0 */

  /* Set the interrupt Threshold control interval to 0 */
  USB_REG(USBPortNum)->USBCMD_D &= ~0x00FF0000;

  /* Configure the Endpoint List Address */
  /* make sure it in on 64 byte boundary !!! */
  /* init list address */
  USB_REG(USBPortNum)->ENDPOINTLISTADDR = (uint32_t)dQueueHead;
  
  /* Enable interrupts: USB interrupt, error, port change, reset, suspend, NAK interrupt */
  USB_REG(USBPortNum)->USBINTR_D =  USBINTR_D_UsbIntEnable | USBINTR_D_UsbErrorIntEnable |
						USBINTR_D_PortChangeIntEnable | USBINTR_D_UsbResetEnable |
						USBINTR_D_SuspendEnable | USBINTR_D_NAKEnable | USBINTR_D_SofReceivedEnable;

  USB_Device_SetDeviceAddress(0);
  
  endpointselected = 0;
  for(i=0;i<ENDPOINT_TOTAL_ENDPOINTS;i++)
	  endpointhandle[i] = 0;

  return;
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
bool Endpoint_ConfigureEndpoint(const uint8_t Number, const uint8_t Type,
							   const uint8_t Direction, const uint16_t Size, const uint8_t Banks)
{
	uint8_t* ISO_Address;
	uint32_t PhyEP = 2*Number + (Direction == ENDPOINT_DIR_OUT ? 0 : 1);
	uint32_t EndPtCtrl = ENDPTCTRL_REG(Number);

	memset(&dQueueHead[PhyEP], 0, sizeof(DeviceQueueHead) );
	
	dQueueHead[PhyEP].MaxPacketSize = Size & 0x3ff;
	dQueueHead[PhyEP].IntOnSetup = 1;
	dQueueHead[PhyEP].ZeroLengthTermination = 1;
	dQueueHead[PhyEP].overlay.NextTD = LINK_TERMINATE;
		
	if (Direction == ENDPOINT_DIR_OUT)
	{
		EndPtCtrl &= ~0x0000FFFF;
		EndPtCtrl |= ((Type << 2) & ENDPTCTRL_RxType) | ENDPTCTRL_RxEnable | ENDPTCTRL_RxToggleReset;
		if(Type == EP_TYPE_ISOCHRONOUS)
		{
			uint32_t size = 0;
			ENDPTCTRL_REG(Number) = (Type << 2);					// TODO dummy to let DcdDataTransfer() knows iso transfer
			ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(Number, &size);
			DcdDataTransfer(PhyEP, ISO_Address,USB_DATA_BUFFER_TEM_LENGTH);
		}
		else
			USB_REG(USBPortNum)->ENDPTNAKEN |=  (1 << EP_Physical2BitPosition(PhyEP));
	}else /* ENDPOINT_DIR_IN */
	{
		EndPtCtrl &= ~0xFFFF0000;
		EndPtCtrl |= ((Type << 18) & ENDPTCTRL_TxType)| ENDPTCTRL_TxEnable| ENDPTCTRL_TxToggleReset;
		if(Type == EP_TYPE_ISOCHRONOUS)
		{
			uint32_t size = 0;
			ENDPTCTRL_REG(Number) = (Type << 18);					// TODO dummy to let DcdDataTransfer() knows iso transfer
			ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(Number, &size);
			DcdDataTransfer(PhyEP, ISO_Address, size);
		}
	}
	ENDPTCTRL_REG(Number) = EndPtCtrl;

	endpointhandle[Number] = (Number==ENDPOINT_CONTROLEP) ? ENDPOINT_CONTROLEP : PhyEP;
	return true;
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void DcdDataTransfer(uint8_t PhyEP, uint8_t *pData, uint32_t length)
{
	DeviceTransferDescriptor*  pDTD = (DeviceTransferDescriptor*) &dTransferDescriptor[ PhyEP ];

	while ( USB_REG(USBPortNum)->ENDPTSTAT & _BIT( EP_Physical2BitPosition(PhyEP) ) )	/* Endpoint is already primed */
	{
	}
	

	/* Zero out the device transfer descriptors */
	memset((void*)pDTD, 0, sizeof(DeviceTransferDescriptor));

	if(((ENDPTCTRL_REG(PhyEP/2)>>2)&EP_TYPE_MASK)==EP_TYPE_ISOCHRONOUS)	// iso out endpoint
	{
		uint32_t mult = (USB_DATA_BUFFER_TEM_LENGTH + 1024) / 1024;
		pDTD->NextTD = LINK_TERMINATE ;
		dQueueHead[PhyEP].Mult = mult;
	}
	else if(((ENDPTCTRL_REG(PhyEP/2)>>18)&EP_TYPE_MASK)==EP_TYPE_ISOCHRONOUS)// iso in endpoint
	{
		uint32_t mult = (USB_DATA_BUFFER_TEM_LENGTH + 1024) / 1024;
		pDTD->NextTD = LINK_TERMINATE ;
		dQueueHead[PhyEP].Mult = mult;
	}
	else																		// other endpoint types
	{
		pDTD->NextTD = LINK_TERMINATE ; /* The next DTD pointer is INVALID */
	}
	pDTD->TotalBytes = length;
	pDTD->IntOnComplete = 1;
	pDTD->Active = 1;

	pDTD->BufferPage[0] = (uint32_t) pData;
	pDTD->BufferPage[1] = ((uint32_t) pData + 0x1000) & 0xfffff000;
	pDTD->BufferPage[2] = ((uint32_t) pData + 0x2000) & 0xfffff000;
	pDTD->BufferPage[3] = ((uint32_t) pData + 0x3000) & 0xfffff000;
	pDTD->BufferPage[4] = ((uint32_t) pData + 0x4000) & 0xfffff000;

	dQueueHead[PhyEP].overlay.Halted = 0; /* this should be in USBInt */
	dQueueHead[PhyEP].overlay.Active = 0; /* this should be in USBInt */
	dQueueHead[PhyEP].overlay.NextTD = (uint32_t) &dTransferDescriptor[ PhyEP ];
	dQueueHead[PhyEP].TransferCount = length;
	
	/* prime the endpoint for transmit */
	USB_REG(USBPortNum)->ENDPTPRIME |= _BIT( EP_Physical2BitPosition(PhyEP) ) ;
}

void TransferCompleteISR(void)
{
	uint8_t* ISO_Address;
	uint32_t ENDPTCOMPLETE = USB_REG(USBPortNum)->ENDPTCOMPLETE;
	USB_REG(USBPortNum)->ENDPTCOMPLETE = ENDPTCOMPLETE;
	
	if (ENDPTCOMPLETE)
	{
		uint8_t n;
		for (n = 0; n < USED_PHYSICAL_ENDPOINTS/2; n++) /* LOGICAL */
		{
			if ( ENDPTCOMPLETE & _BIT(n) ) /* OUT */
			{
				if(((ENDPTCTRL_REG(n)>>2)&EP_TYPE_MASK)==EP_TYPE_ISOCHRONOUS)	// iso out endpoint
				{
					uint32_t size = dQueueHead[2*n].TransferCount - dQueueHead[2*n].overlay.TotalBytes;
					// copy to share buffer
					ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(n, &size);
					DcdDataTransfer(2*n, ISO_Address,USB_DATA_BUFFER_TEM_LENGTH);
				}
				else
				{
					dQueueHead[2*n].TransferCount -= dQueueHead[2*n].overlay.TotalBytes;
					dQueueHead[2*n].IsOutReceived = 1;
					usb_data_buffer_size = dQueueHead[2*n].TransferCount;
				}
			}
			if ( ENDPTCOMPLETE & _BIT( (n+16) ) ) /* IN */
			{
				if(((ENDPTCTRL_REG(n)>>18)&EP_TYPE_MASK)==EP_TYPE_ISOCHRONOUS)	// iso in endpoint
				{
					uint32_t size;
					ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(n, &size);
					DcdDataTransfer(2*n +1, ISO_Address, size);
				}
			}
		}
	}
}


void Endpoint_GetSetupPackage(uint8_t* pData)
{
	USB_Request_Header_t *ctrlrq = (USB_Request_Header_t*)pData;
	memcpy(pData, (void*)dQueueHead[0].SetupPackage, 8);
	/* Below fix is to prevent Endpoint_Read_Control_Stream_LE()
	 * from getting wrong data*/

	if(
			(ctrlrq->wLength != 0)
	)
	{
		dQueueHead[0].IsOutReceived = 0;
	}
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void DcdIrqHandler (uint8_t HostID)
{
	uint32_t USBSTS_D;

	USBSTS_D = USB_REG(USBPortNum)->USBSTS_D & USB_REG(USBPortNum)->USBINTR_D;                      /* Device Interrupt Status */
	if (USBSTS_D == 0)	/* avoid to clear disabled interrupt source */
		return;

	USB_REG(USBPortNum)->USBSTS_D = USBSTS_D; /* Acknowledge Interrupt */
	
	/* Process Interrupt Sources */	
	if (USBSTS_D & USBSTS_D_UsbInt)
	{		
		if (USB_REG(USBPortNum)->ENDPTSETUPSTAT)
		{
//			memcpy(SetupPackage, dQueueHead[0].SetupPackage, 8);
			/* Will be cleared by Endpoint_ClearSETUP */
		}
		
		if (USB_REG(USBPortNum)->ENDPTCOMPLETE)
		{
			TransferCompleteISR();
		}
	}
	
	if (USBSTS_D & USBSTS_D_NAK)					/* NAK */
	{
		uint32_t ENDPTNAK = USB_REG(USBPortNum)->ENDPTNAK & USB_REG(USBPortNum)->ENDPTNAKEN;
		USB_REG(USBPortNum)->ENDPTNAK = ENDPTNAK;

	    if (ENDPTNAK)  /* handle NAK interrupts */
	    {
			uint8_t LogicalEP;
			for (LogicalEP = 0; LogicalEP < USED_PHYSICAL_ENDPOINTS / 2; LogicalEP++)
			{
				if (ENDPTNAK & _BIT(LogicalEP)) /* Only OUT Endpoint is NAK enable */
				{
					uint8_t PhyEP = 2*LogicalEP;
					if ( ! (USB_REG(USBPortNum)->ENDPTSTAT & _BIT(LogicalEP)) ) /* Is In ready */
					{
						/* Check read OUT flag */
						if(!dQueueHead[PhyEP].IsOutReceived)
						{
							usb_data_buffer_size = 0;
							DcdDataTransfer(PhyEP, usb_data_buffer, 512);
						}
					}
				}
			}
	    }
	}
	
	if (USBSTS_D & USBSTS_D_SofReceived)					/* Start of Frame Interrupt */
	{
		EVENT_USB_Device_StartOfFrame();
	}

	if (USBSTS_D & USBSTS_D_ResetReceived)                      /* Reset */
	{
		HAL_Reset();
		USB_DeviceState = DEVICE_STATE_Default;
		Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL, ENDPOINT_DIR_OUT, USB_Device_ControlEndpointSize,0);
		Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL, ENDPOINT_DIR_IN, USB_Device_ControlEndpointSize, 0);
	}

	if (USBSTS_D & USBSTS_D_SuspendInt)                   /* Suspend */
	{

	}

	if (USBSTS_D & USBSTS_D_PortChangeDetect)                  /* Resume */
	{

	}

	if (USBSTS_D & USBSTS_D_UsbErrorInt)					/* Error Interrupt */
	{
		//while(1){}
	}
}

uint32_t Dummy_EPGetISOAddress(uint32_t EPNum, uint32_t* last_packet_size)
{
	return (uint32_t)iso_buffer;
}

#endif /*__LPC18XX__*/
