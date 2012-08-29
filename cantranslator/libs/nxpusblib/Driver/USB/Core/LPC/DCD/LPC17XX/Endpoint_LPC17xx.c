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


#if (defined(__LPC17XX__)||defined(__LPC177X_8X__)) && defined(USB_CAN_BE_DEVICE)
#include "../../../Endpoint.h"

#define IsOutEndpoint(PhysicalEP)		(! ((PhysicalEP) & 1) )

volatile bool SETUPReceived;
bool isOutReceived;
bool isInReady;

uint32_t UDCA[32] __DATA(USBRAM_SECTION) ATTR_ALIGNED(128);
DMADescriptor dmaDescriptor[USED_PHYSICAL_ENDPOINTS] __DATA(USBRAM_SECTION);
static uint8_t SetupPackage[8] __DATA(USBRAM_SECTION);
uint32_t DataInRemainCount,DataInRemainOffset;
bool IsConfigured,shortpacket;
uint8_t* ISO_Address;
uint8_t iso_buffer[512] ATTR_ALIGNED(4) __DATA(USBRAM_SECTION);
uint32_t CALLBACK_HAL_GetISOBufferAddress(const uint32_t EPNum,uint32_t* last_packet_size) ATTR_WEAK ATTR_ALIAS(Dummy_EPGetISOAddress);
uint32_t BufferAddressIso[32] __DATA(USBRAM_SECTION);
uint32_t SizeAudioTransfer;

/*
 *  Write Command
 *    Parameters:      cmd:   Command
 *    Return Value:    None
 */

void SIE_WriteCommamd (uint32_t cmd) {

  LPC_USB->USBDevIntClr = CCEMTY_INT;
  LPC_USB->USBCmdCode = cmd;
  while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void SIE_WriteCommandData (uint32_t cmd, uint32_t val) {

	LPC_USB->USBDevIntClr = CCEMTY_INT;
	LPC_USB->USBCmdCode = cmd;
	while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
	LPC_USB->USBDevIntClr = CCEMTY_INT;
	LPC_USB->USBCmdCode = val;
	while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
uint32_t SIE_ReadCommandData (uint32_t cmd) {

	LPC_USB->USBDevIntClr = CCEMTY_INT | CDFULL_INT;
	LPC_USB->USBCmdCode = cmd;
	while ((LPC_USB->USBDevIntSt & CDFULL_INT) == 0);
  	return (LPC_USB->USBCmdData);
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_Reset (void)
{
	uint32_t n;

	LPC_USB->USBEpInd = 0;
	LPC_USB->USBMaxPSize = USB_Device_ControlEndpointSize;
	LPC_USB->USBEpInd = 1;
	LPC_USB->USBMaxPSize = USB_Device_ControlEndpointSize;
	while ((LPC_USB->USBDevIntSt & EP_RLZED_INT) == 0);

	/* Slave Register */
	LPC_USB->USBEpIntEn		= 0;
	LPC_USB->USBDevIntEn	= (DEV_STAT_INT | EP_SLOW_INT | ERR_INT);

	LPC_USB->USBEpIntClr	= 0xFFFFFFFF;
	LPC_USB->USBDevIntClr	= 0xFFFFFFFF;
	LPC_USB->USBEpIntPri	= 0;

	/* DMA registers */
	LPC_USB->USBEpDMADis	= 0xFFFFFFFF;
	LPC_USB->USBDMARClr		= 0xFFFFFFFF;
	LPC_USB->USBEoTIntClr	= 0xFFFFFFFF;
	LPC_USB->USBNDDRIntClr	= 0xFFFFFFFF;
	LPC_USB->USBSysErrIntClr = 0xFFFFFFFF;

	LPC_USB->USBDMAIntEn  = (EOT_INT | NDD_REQ_INT | SYS_ERR_INT );
	LPC_USB->USBUDCAH   = (uint32_t) UDCA;
	for (n = 0; n < USED_PHYSICAL_ENDPOINTS; n++) {
		UDCA[n] = 0;
	}
	IsConfigured = false;
	isOutReceived = false;
	isInReady = true;
//	SIE_WriteCommandData(CMD_SET_MODE, DAT_WR_BYTE(INAK_IO | INAK_BO) ); /* Disable INAK_IO, INAK_BO */
}

/**
 *
 */
void Endpoint_StallTransaction(void)
{
	if(endpointselected==ENDPOINT_CONTROLEP)
		SIE_WriteCommandData( CMD_SET_EP_STAT(endpointhandle[endpointselected]), DAT_WR_BYTE(EP_STAT_CND_ST) );
	else
		SIE_WriteCommandData( CMD_SET_EP_STAT(endpointhandle[endpointselected]), DAT_WR_BYTE(EP_STAT_ST) );
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

	if((!IsConfigured)&&(PhyEP>1))
	{
		IsConfigured = true;
		SIE_WriteCommandData(CMD_CFG_DEV, DAT_WR_BYTE(CONF_DVICE));
	}

	LPC_USB->USBReEp |= (1 << PhyEP); 		/* Realize endpoint */

	LPC_USB->USBEpInd = PhyEP;				/* Endpoint Index */
	LPC_USB->USBMaxPSize = Size & 0x3ff;	/* Max Packet Size */

	while ((LPC_USB->USBDevIntSt & EP_RLZED_INT) == 0);		/* TODO shouldd we wait for this */
	LPC_USB->USBDevIntClr = EP_RLZED_INT;
	
	if (Number == ENDPOINT_CONTROLEP) /* Control endpoints have to uses slave mode */
	{
		LPC_USB->USBEpIntEn |= _BIT(PhyEP);
		DataInRemainCount = 0;
		DataInRemainOffset = 0;
	}else /* all other endpoints use DMA mode */
	{
		memset(&dmaDescriptor[PhyEP], 0, sizeof(DMADescriptor));
		dmaDescriptor[PhyEP].Isochronous = (Type == EP_TYPE_ISOCHRONOUS ? 1 : 0 );
		dmaDescriptor[PhyEP].MaxPacketSize = Size;
		dmaDescriptor[PhyEP].Retired = 1; /* inactive DD */
		
		LPC_USB->USBEpDMAEn = _BIT(PhyEP);
	}
	
	SIE_WriteCommandData(CMD_SET_EP_STAT(PhyEP), DAT_WR_BYTE(0)); /*enable endpoint*/
	SIE_WriteCommandData(CMD_SET_EP_STAT(PhyEP), DAT_WR_BYTE(0)); /* Reset Endpoint */

	endpointhandle[Number] = (Number==ENDPOINT_CONTROLEP) ? ENDPOINT_CONTROLEP : PhyEP;
	return true;
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void ReadControlEndpoint( uint8_t *pData )
{
	uint32_t cnt, n;

	LPC_USB->USBCtrl = CTRL_RD_EN;

	do
	{
		cnt = LPC_USB->USBRxPLen;
	} while ((cnt & PKT_RDY) == 0);
	cnt &= PKT_LNGTH_MASK;

	for (n = 0; n < (cnt + 3) / 4; n++)
	{
		*((uint32_t *)pData) = LPC_USB->USBRxData;
		pData += 4;
	}
	LPC_USB->USBCtrl = 0;

	if(cnt >0 && (SETUPReceived == false))
	{
		isOutReceived = true;
	}

//	SIE_WriteCommamd(CMD_SEL_EP(ENDPOINT_CONTROLEP));
//	SIE_WriteCommamd(CMD_CLR_BUF);
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void WriteControlEndpoint( uint8_t *pData, uint32_t cnt )
{
	uint32_t n;
	uint32_t count;

	isInReady = false;
	if(cnt >= USB_Device_ControlEndpointSize)
	{
		if(cnt == USB_Device_ControlEndpointSize)
		{
			shortpacket = true;
		}
		count = USB_Device_ControlEndpointSize;
		DataInRemainCount = cnt - USB_Device_ControlEndpointSize;
		DataInRemainOffset += count;
	}
	else
	{
		count = cnt;
		DataInRemainCount = 0;
		DataInRemainOffset = 0;
	}
	LPC_USB->USBCtrl = CTRL_WR_EN;
	LPC_USB->USBTxPLen = count;

	for (n = 0; n < (count + 3) / 4; n++)
	{
		LPC_USB->USBTxData = *((uint32_t *)pData);
		pData += 4;
	}

	LPC_USB->USBCtrl = 0;

	SIE_WriteCommamd(CMD_SEL_EP(ENDPOINT_CONTROLEP+1));
	SIE_WriteCommamd(CMD_VALID_BUF);
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL17XX_SetDeviceAddress (uint8_t Address)
{
	SIE_WriteCommandData(CMD_SET_ADDR, DAT_WR_BYTE(DEV_EN | Address)); /* Don't wait for next */
	SIE_WriteCommandData(CMD_SET_ADDR, DAT_WR_BYTE(DEV_EN | Address)); /*  Setup Status Phase */
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL17XX_USBConnect (uint32_t con)
{
	SIE_WriteCommandData(CMD_SET_DEV_STAT, DAT_WR_BYTE(con ? DEV_CON : 0));
}

void Endpoint_GetSetupPackage(uint8_t* pData)
{
	memcpy(pData, SetupPackage, 8);
}

void SlaveEndpointISR() 
{
	uint32_t PhyEP;
	for (PhyEP = 0; PhyEP < 2; PhyEP++)		  /* Check Control Endpoints */
	{
		if (LPC_USB->USBEpIntSt & _BIT(PhyEP))
		{
			LPC_USB->USBEpIntClr = _BIT(PhyEP);	/*-- Clear Interrupt Endpoint --*/

			if (PhyEP == ENDPOINT_CONTROLEP)            /* Control OUT Endpoint */
			{
				uint32_t SIEEndpointStatus;
				
				while ((LPC_USB->USBDevIntSt & CDFULL_INT) == 0);
				SIEEndpointStatus = LPC_USB->USBCmdData;
				
				if (SIEEndpointStatus & EP_SEL_STP)     /* Setup Packet */
				{
					SETUPReceived = true;
					ReadControlEndpoint(SetupPackage);
				}else
				{
					ReadControlEndpoint(usb_data_buffer);
				}
			}
			else                              /* IN Endpoint */
			{
				isInReady = true;
				if(DataInRemainCount)
				{
					WriteControlEndpoint((uint8_t*)(usb_data_buffer+DataInRemainOffset),DataInRemainCount);
				}
				else
				{
					if(shortpacket)
					{
						shortpacket = false;
						WriteControlEndpoint((uint8_t*)(usb_data_buffer+DataInRemainOffset),DataInRemainCount);
						DataInRemainOffset = 0;
					}
				}
			}
		}
	}
}

void DcdDataTransfer(uint8_t PhyEP, uint8_t *pData, uint32_t cnt)
{
	dmaDescriptor[PhyEP].BufferStartAddr = pData;
	if(dmaDescriptor[PhyEP].Isochronous == 1) // iso endpoint
	{
		if(PhyEP&1) // IN DIRECTION
		{
			uint8_t BufferCount;
			for (BufferCount = 0; BufferCount < cnt/0xFF; BufferCount ++)
			{
				BufferAddressIso[BufferCount] = 0xFF;
			}
			BufferAddressIso[BufferCount] = (cnt % 0xFF);
			if(cnt % 0xFF != 0)
			{
				dmaDescriptor[PhyEP].BufferLength = cnt/0xFF + 1;
			}
			else
			dmaDescriptor[PhyEP].BufferLength = cnt/0xFF;
		}
		else  // OUT DIRECTION
		{
			dmaDescriptor[PhyEP].BufferLength = 1;
		}
		dmaDescriptor[PhyEP].IsoBufferAddr = (uint32_t)BufferAddressIso;
		dmaDescriptor[PhyEP].Isochronous = 1;
		dmaDescriptor[PhyEP].MaxPacketSize = 0;
	}
	else
	{
	dmaDescriptor[PhyEP].BufferLength = cnt;
	}
	dmaDescriptor[PhyEP].Retired = 0;
	dmaDescriptor[PhyEP].Status = 0;
	dmaDescriptor[PhyEP].IsoPacketValid = 0;
	dmaDescriptor[PhyEP].LSByteExtracted = 0;
	dmaDescriptor[PhyEP].MSByteExtracted = 0;
	dmaDescriptor[PhyEP].PresentCount = 0;
	
	
	UDCA[PhyEP] = (uint32_t) &dmaDescriptor[PhyEP];
	LPC_USB->USBEpDMAEn = _BIT(PhyEP);
}
void DMAEndTransferISR() 
{
	uint32_t PhyEP;
	uint32_t EoTIntSt = LPC_USB->USBEoTIntSt;

	for (PhyEP = 2; PhyEP < USED_PHYSICAL_ENDPOINTS; PhyEP++)              /* Check All Endpoints */
	{
		if ( EoTIntSt & _BIT(PhyEP) )
		{
			if ( IsOutEndpoint(PhyEP) )                 /* OUT Endpoint */
			{
				if(dmaDescriptor[PhyEP].Isochronous == 1) // iso endpoint
				{
					SizeAudioTransfer = (BufferAddressIso[0])& 0xFFFF;
					ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(PhyEP/2,&SizeAudioTransfer);
					DcdDataTransfer(PhyEP, ISO_Address,512);
				}
				usb_data_buffer_size = dmaDescriptor[PhyEP].PresentCount;
			}
			else			                    /* IN Endpoint */
			{
				/* Should be left blank */
			}
		}
	}
	LPC_USB->USBEoTIntClr = EoTIntSt;
}

void DMANewTransferRequestISR() 
{
	uint32_t PhyEP;
	uint32_t NDDRIntSt = LPC_USB->USBNDDRIntSt;

	for (PhyEP = 2; PhyEP < USED_PHYSICAL_ENDPOINTS; PhyEP++)              /* Check All Endpoints */
	{
		if ( NDDRIntSt & _BIT(PhyEP) )
		{
			if ( IsOutEndpoint(PhyEP) )                     /* OUT Endpoint */
			{
				if(dmaDescriptor[PhyEP].Isochronous == 1) // iso endpoint
				{
					DcdDataTransfer(PhyEP, ISO_Address,512);
				}
				else
				DcdDataTransfer(PhyEP, usb_data_buffer, 512);
			}
			else			                    /* IN Endpoint */
			{
				if(dmaDescriptor[PhyEP].Isochronous == 1)
				{
					ISO_Address = (uint8_t*)CALLBACK_HAL_GetISOBufferAddress(PhyEP/2,&SizeAudioTransfer);
					if(SizeAudioTransfer > 0)
						DcdDataTransfer(PhyEP, ISO_Address,SizeAudioTransfer);
					else
						DcdDataTransfer(PhyEP, ISO_Address,512);
				}
			}
		}
	}
	LPC_USB->USBNDDRIntClr = NDDRIntSt;
}

// void DMASysErrISR() 
// {
// 	uint32_t PhyEP;
// 	uint32_t SysErrIntSt = LPC_USB->USBSysErrIntSt;
// 	for (PhyEP = 2; PhyEP < USED_PHYSICAL_ENDPOINTS; PhyEP++)              /* Check All Endpoints */
// 	{
// 		if ( SysErrIntSt & _BIT(PhyEP) )
// 		{
// 			if ( IsOutEndpoint(PhyEP) )         /* OUT Endpoint */
// 			{
// 			}
// 			else			                    /* IN Endpoint */
// 			{
// 			}
// 		}
// 	}
// 	LPC_USB->USBSysErrIntClr = SysErrIntSt;
// }

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void DcdIrqHandler (uint8_t DeviceID)
{
	uint32_t DevIntSt, DMAIntSt;

	DevIntSt = LPC_USB->USBDevIntSt & LPC_USB->USBDevIntEn;                      /* Device Interrupt Status */
	LPC_USB->USBDevIntClr = DevIntSt;

	/* Device Status Interrupt (Reset, Connect change, Suspend/Resume) */
	if (DevIntSt & DEV_STAT_INT)
	{
		uint32_t SIEDeviceStatus;
		SIE_WriteCommamd(CMD_GET_DEV_STAT);
		SIEDeviceStatus = SIE_ReadCommandData(DAT_GET_DEV_STAT);       /* Device Status */
		if (SIEDeviceStatus & DEV_RST)	                    /* Reset */
		{
			HAL_Reset();
			USB_DeviceState = DEVICE_STATE_Default;
			Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, 0, ENDPOINT_DIR_OUT, USB_Device_ControlEndpointSize,0);
			Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, 0, ENDPOINT_DIR_IN, USB_Device_ControlEndpointSize,0);
		}
		if (SIEDeviceStatus & DEV_CON_CH)	                /* Connect change */
		{
		}
		if (SIEDeviceStatus & DEV_SUS_CH)                   /* Suspend/Resume */
		{
			if (SIEDeviceStatus & DEV_SUS)                  /* Suspend */
			{
			}
			else                                /* Resume */
			{
			}
		}
	}

	if (DevIntSt & FRAME_INT)
	{

	}

	if (DevIntSt & ERR_INT)
	{
		volatile uint32_t SIEErrorStatus;
		SIE_WriteCommamd(CMD_RD_ERR_STAT);
		SIEErrorStatus = SIE_ReadCommandData(DAT_RD_ERR_STAT);
	}

	/* SLAVE mode : Endpoint's Slow Interrupt */
	if ( (DevIntSt & EP_SLOW_INT) || (DevIntSt & EP_FAST_INT) )
	{
		SlaveEndpointISR();
	}

	/* DMA mode */
	DMAIntSt = LPC_USB->USBDMAIntSt & LPC_USB->USBDMAIntEn;
	
	if (DMAIntSt & EOT_INT)            /* End of Transfer Interrupt */
	{
		DMAEndTransferISR();

	}

	if (DMAIntSt & NDD_REQ_INT)            /* New DD Request Interrupt */
	{
		DMANewTransferRequestISR();

	}

	if (DMAIntSt & SYS_ERR_INT)            /* System Error Interrupt */
	{
		// DMASysErrISR();
	}
}
uint32_t Dummy_EPGetISOAddress(uint32_t EPNum, uint32_t last_packet_size)
{
	return (uint32_t)iso_buffer;
}
#endif /*__LPC17XX__*/
