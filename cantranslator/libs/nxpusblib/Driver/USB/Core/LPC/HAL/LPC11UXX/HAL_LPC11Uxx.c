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

#if defined(__LPC11UXX__)

#include "../HAL_LPC.h"
#include "../../../USBTask.h"


/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
 void HAL_USBInit(void)
 {
  /* Enable AHB clock to the USB block and USB RAM. */
  LPC_SYSCON->SYSAHBCLKCTRL |= ((0x1<<14)|(0x1<<27));

  LPC_USB->EPBUFCFG = 0x3FC;

  /* configure usb_soft connect */
  LPC_IOCON->PIO0_6 = 0x01;

  HAL_Reset();
  return;
 }
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
 void HAL_USBDeInit(void)
 {
	 NVIC_DisableIRQ(USB_IRQn);								/* disable USB interrupt */
	 LPC_SYSCON->SYSAHBCLKCTRL &= ~((0x1<<14)|(0x1<<27));	/* disable USB clock     */
 }
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_EnableUSBInterrupt(void)
{
	NVIC_EnableIRQ(USB_IRQn);
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_DisableUSBInterrupt(void)
{
	NVIC_DisableIRQ(USB_IRQn);
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_SetDeviceAddress (uint8_t Address)
{
#ifdef USB_CAN_BE_DEVICE
	LPC_USB->DEVCMDSTAT &= ~0x7F;
	LPC_USB->DEVCMDSTAT |= (USB_EN | Address);
#endif
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_USBConnect (uint32_t con)
{
#ifdef USB_CAN_BE_DEVICE
	if ( con )
	{
		LPC_USB->DEVCMDSTAT |= USB_DCON;
	}
	else
	{
		LPC_USB->DEVCMDSTAT &= ~USB_DCON;
	}
#endif
}
#endif /*__LPC11UXX__*/
