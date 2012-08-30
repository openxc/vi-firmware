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

#if defined(__LPC11UXX__) && defined(USB_CAN_BE_DEVICE)
#include "../../../Endpoint.h"

#define IsOutEndpoint(PhysicalEP)		(! ((PhysicalEP) & 1) )
/*static*/ USB_CMD_STAT EndPointCmdStsList[10][2] __DATA(USBRAM_SECTION) ATTR_ALIGNED(256) ; /* 10 endpoints with 2 buffers each */
static uint8_t SetupPackage[8] __DATA(USBRAM_SECTION) ATTR_ALIGNED(64);
uint32_t EndpointMaxPacketSize[10];
uint32_t Remain_length;
bool shortpacket;
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_Reset (void)
{
	LPC_USB->EPINUSE = 0;
	LPC_USB->EPSKIP = 0xFFFFFFFF;
	LPC_USB->EPBUFCFG = 0;
	
	LPC_USB->DEVCMDSTAT |= USB_EN | USB_IntOnNAK_AO | USB_IntOnNAK_CO;
	/* Clear all EP interrupts, device status, and SOF interrupts. */
	LPC_USB->INTSTAT = 0xC00003FF;
	/* Enable all ten(10) EPs interrupts including EP0, note: EP won't be
	ready until it's configured/enabled when device sending SetEPStatus command
	to the command engine. */
	LPC_USB->INTEN  = DEV_STAT_INT;

	/* Initialize EP Command/Status List. */
	LPC_USB->EPLISTSTART = (uint32_t) EndPointCmdStsList;
	LPC_USB->DATABUFSTART = ((uint32_t) usb_data_buffer) & 0xFFC00000;
	
	memset(EndPointCmdStsList, 0, sizeof(EndPointCmdStsList) );
	
	HAL_SetDeviceAddress(0);
	
	return;
}

bool Endpoint_ConfigureEndpointControl(const uint16_t Size)
{
	/* Endpoint Control OUT Buffer 0 */
	EndPointCmdStsList[0][0].BufferAddrOffset = 0;
	EndPointCmdStsList[0][0].NBytes = 0x3FF;
	EndPointCmdStsList[0][0].Active = 0;
	
	/* Setup Buffer */
	EndPointCmdStsList[0][1].BufferAddrOffset = ( ((uint32_t) SetupPackage) >> 6) & 0xFFFF;

	/* Endpoint Control IN Buffer 0 */
	EndPointCmdStsList[1][0].BufferAddrOffset = 0;
	EndPointCmdStsList[1][0].NBytes = 0;
	EndPointCmdStsList[1][0].Active = 0;

	LPC_USB->INTSTAT &= ~3;
	LPC_USB->INTEN |= 3;

	EndpointMaxPacketSize[0]=EndpointMaxPacketSize[1]=Size;

	return true;
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
bool Endpoint_ConfigureEndpoint(const uint8_t Number, const uint8_t Type,
								const uint8_t Direction, const uint16_t Size, const uint8_t Banks)
{
	uint32_t PhyEP = 2*Number + (Direction == ENDPOINT_DIR_OUT ? 0 : 1);
	
	memset(EndPointCmdStsList[PhyEP], 0, sizeof(USB_CMD_STAT)*2 );
	EndPointCmdStsList[PhyEP][0].NBytes = IsOutEndpoint(PhyEP) ? 0x3FF : 0;	
	
	LPC_USB->INTSTAT &= ~ (1 << PhyEP);
	LPC_USB->INTEN |= (1 << PhyEP);

	EndpointMaxPacketSize[PhyEP] = Size;
	endpointhandle[Number] = (Number==ENDPOINT_CONTROLEP) ? ENDPOINT_CONTROLEP : PhyEP;

	return true;
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void DcdDataTransfer( uint8_t EPNum, uint8_t *pData, uint32_t length )
{
	if ((EPNum&1)&&(length >= EndpointMaxPacketSize[EPNum]))
	{
		if((length == EndpointMaxPacketSize[EPNum])&&(EPNum==1))
		{
			shortpacket = true;
		}
		Remain_length = length - EndpointMaxPacketSize[EPNum];
		length = EndpointMaxPacketSize[EPNum];
	}
	else
	{
		Remain_length = 0;
	}

	if(EPNum&1)
	{
		EndPointCmdStsList[EPNum][0].NBytes = length;
	}
	EndPointCmdStsList[EPNum][0].BufferAddrOffset = ( ((uint32_t) pData) >> 6 ) & 0xFFFF;
	
	EndPointCmdStsList[EPNum][0].Active = 1;
}

void Endpoint_GetSetupPackage(uint8_t* pData)
{
	memcpy(pData, SetupPackage, 8);
	/* Clear endpoint control stall flag if set */
	EndPointCmdStsList[0][0].Stall = 0;
	EndPointCmdStsList[1][0].Stall = 0;
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void USB_IRQHandler (void)
{
	uint32_t IntStat = LPC_USB->INTSTAT & LPC_USB->INTEN;         /* Get Interrupt Status and clear immediately. */
	
	if (IntStat == 0)
		return;
	
	LPC_USB->INTSTAT = IntStat;
	
	/* SOF Interrupt */
	if (IntStat & FRAME_INT)
	{
	
	}
	
	/* Device Status Interrupt (Reset, Connect change, Suspend/Resume) */
	if (IntStat & DEV_STAT_INT)
	{
		uint32_t DevCmdStat = LPC_USB->DEVCMDSTAT;       		/* Device Status */

		if (DevCmdStat & USB_DRESET_C)               	/* Reset */
		{
		  LPC_USB->DEVCMDSTAT |= USB_DRESET_C;
		  HAL_Reset();
		  USB_DeviceState = DEVICE_STATE_Default;
		  Endpoint_ConfigureEndpointControl(USB_Device_ControlEndpointSize);
		}
		
		if (DevCmdStat & USB_DCON_C)                 	/* Connect change */
		{
		  LPC_USB->DEVCMDSTAT |= USB_DCON_C;
		}
		
		if (DevCmdStat & USB_DSUS_C)                   /* Suspend/Resume */
		{
		  LPC_USB->DEVCMDSTAT |= USB_DSUS_C;
		  if (DevCmdStat & USB_DSUS)                   /* Suspend */
		  {
		  }
		  else                               	/* Resume */
		  {
		  }
		}
	}

	/* Endpoint's Interrupt */
	if (IntStat & 0x3FF) {  /* if any of the EP0 through EP9 is set, or bit 0 through 9 on disr */
		uint32_t PhyEP;
		for (PhyEP = 0; PhyEP < USED_PHYSICAL_ENDPOINTS; PhyEP++) /* Check All Endpoints */
		{
			if ( IntStat & (1 << PhyEP) )
			{
				if ( IsOutEndpoint(PhyEP) ) /* OUT Endpoint */
				{                 
					if ( !Endpoint_IsSETUPReceived() ){
						if(PhyEP <= 1)
						{
						DcdDataTransfer(PhyEP, usb_data_buffer, 512);
						}
						else
						{
							usb_data_buffer_size = (1023 - EndPointCmdStsList[PhyEP][0].NBytes);
							if(EndPointCmdStsList[PhyEP][0].NBytes == 0x3FF)
							{
								DcdDataTransfer(PhyEP, usb_data_buffer, 512);
							}
						}
					}
				} else                             /* IN Endpoint */
				{
					if (Remain_length > 0)
					{
						uint32_t i;
						for (i = 0; i < Remain_length; i++)
						{
							usb_data_buffer [i] = usb_data_buffer [i + EndpointMaxPacketSize[PhyEP]];
						}
						DcdDataTransfer(PhyEP,usb_data_buffer, Remain_length);
					}
					else
					{
						if(PhyEP == 1) /* Control IN */
						{
							if(shortpacket)
							{
								shortpacket = false;
								DcdDataTransfer(PhyEP, usb_data_buffer, 0);
							}
						}
					}
				}
			}
		}
	}

	return;
}

#endif /*__LPC11UXX__*/
