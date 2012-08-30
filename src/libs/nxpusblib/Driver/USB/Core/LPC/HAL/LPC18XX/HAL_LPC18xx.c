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

#if defined(__LPC18XX__)||defined(__LPC43XX__)

#include "../HAL_LPC.h"
#if defined(__LPC18XX__)
#include "lpc18xx_cgu.h"
#endif
#if defined(__LPC43XX__)
#include "lpc43xx_cgu.h"
#endif
#include "../../../USBTask.h"

#if defined(USB_CAN_BE_DEVICE)
#include "../../../Device.h"
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_USBConnect (uint32_t con)
{
#if defined(USB_DEVICE_ROM_DRIVER)
  if (con)
	  USBD_API->hw->Connect(UsbHandle, 1);
  else
	  USBD_API->hw->Connect(UsbHandle, 0);
#else
  if (con)
	  USB_REG(USBPortNum)->USBCMD_D |= (1<<0);
  else
	  USB_REG(USBPortNum)->USBCMD_D &= ~(1<<0);
#endif
}
#endif

/* Assumed Xtal Frequency */
#define XTAL_FREQ	12000000

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
 void HAL_USBInit(void)
 {
	 if(USBPortNum)
	 {
		/* connect CLK_USB1 to 60 MHz clock */
		CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_USB1); /* FIXME Run base BASE_USB1_CLK clock from PLL1 (assume PLL1 is 60 MHz, no division required) */
	    /* Turn on the phy */
	    LPC_CREG->CREG0 &= ~(1<<5);
		#if defined(USB_CAN_BE_HOST)
			/* enable USB1_DP and USB1_DN on chip FS phy */
			LPC_SCU->SFSUSB = 0x16;
		#endif
		#if defined(USB_CAN_BE_DEVICE)
			/* enable USB1_DP and USB1_DN on chip FS phy */
			LPC_SCU->SFSUSB = 0x12;
		#endif
			LPC_USB1->PORTSC1_D |= (1<<24);
	 }
	 else
	 {
		 /* Set up USB0 clock */
		 /* Disable PLL first */
		 CGU_EnableEntity(CGU_CLKSRC_PLL0, DISABLE);
		 /* the usb core require output clock = 480MHz */
		#if defined(__LPC18XX__)
		 if(CGU_SetPLL0(XTAL_FREQ, 480000000, 0.98, 1.02) != CGU_ERROR_SUCCESS)
			 while(1);
		#elif defined(__LPC43XX__)
		 if(CGU_SetPLL0() != CGU_ERROR_SUCCESS)
			 while(1);
		#endif
		 CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_CLKSRC_PLL0);
		 /* Enable PLL after all setting is done */
		 CGU_EnableEntity(CGU_CLKSRC_PLL0, ENABLE);
		/* Turn on the phy */
		LPC_CREG->CREG0 &= ~(1<<5);
	 }

#if defined(USB_CAN_BE_DEVICE)&&(!defined(USB_DEVICE_ROM_DRIVER))
	/* reset the controller */
	USB_REG(USBPortNum)->USBCMD_D = USBCMD_D_Reset;
	/* wait for reset to complete */
	while (USB_REG(USBPortNum)->USBCMD_D & USBCMD_D_Reset);

	/* Program the controller to be the USB device controller */
	USB_REG(USBPortNum)->USBMODE_D =   (0x2<<0)/*| (1<<4)*//*| (1<<3)*/ ;
	if(USBPortNum==0)
	{
		/* set OTG transcever in proper state, device is present
		on the port(CCS=1), port enable/disable status change(PES=1). */
		LPC_USB0->OTGSC = (1<<3) | (1<<0) /*| (1<<16)| (1<<24)| (1<<25)| (1<<26)| (1<<27)| (1<<28)| (1<<29)| (1<<30)*/;
		#if (USB_FORCED_FULLSPEED)
			LPC_USB0->PORTSC1_D |= (1<<24);
		#endif
	}
	HAL_Reset();
#endif
 }
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
 void HAL_USBDeInit(void)
 {
	HAL_DisableUSBInterrupt();
#if defined(USB_CAN_BE_HOST)
	USB_REG(USBPortNum)->USBSTS_H = 0xFFFFFFFF; 			/* clear all current interrupts */
	USB_REG(USBPortNum)->PORTSC1_H &= ~(1<<12);				/* clear port power */
	USB_REG(USBPortNum)->USBMODE_H =   (1<<0);				/* set USB mode reserve */
#endif

#if defined(USB_CAN_BE_DEVICE)
	/* Clear all pending interrupts */
	USB_REG(USBPortNum)->USBSTS_D   = 0xFFFFFFFF;
	USB_REG(USBPortNum)->ENDPTNAK   = 0xFFFFFFFF;
	USB_REG(USBPortNum)->ENDPTNAKEN = 0;
	USB_REG(USBPortNum)->ENDPTSETUPSTAT = USB_REG(USBPortNum)->ENDPTSETUPSTAT;
	USB_REG(USBPortNum)->ENDPTCOMPLETE  = USB_REG(USBPortNum)->ENDPTCOMPLETE;
	while (USB_REG(USBPortNum)->ENDPTPRIME);                  /* Wait until all bits are 0 */
	USB_REG(USBPortNum)->ENDPTFLUSH = 0xFFFFFFFF;
	while (USB_REG(USBPortNum)->ENDPTFLUSH); /* Wait until all bits are 0 */
#endif
 }
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_EnableUSBInterrupt(void)
{
	NVIC_EnableIRQ((USBPortNum)?USB1_IRQn:USB0_IRQn); //  enable USB interrupts
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void HAL_DisableUSBInterrupt(void)
{
	NVIC_DisableIRQ((USBPortNum)?USB1_IRQn:USB0_IRQn); //  disable USB interrupts
}

/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void USB0_IRQHandler (void)
{
#ifdef USB_CAN_BE_HOST
	HcdIrqHandler(0);
#endif

#ifdef USB_CAN_BE_DEVICE
	#ifdef USB_DEVICE_ROM_DRIVER
		UsbdRom_IrqHandler();
	#else
		DcdIrqHandler(0);
	#endif
#endif
	return;
}
/********************************************************************//**
 * @brief
 * @param
 * @return
 *********************************************************************/
void USB1_IRQHandler (void)
{
#ifdef USB_CAN_BE_HOST
	HcdIrqHandler(1);
#endif

#ifdef USB_CAN_BE_DEVICE
#ifdef USB_DEVICE_ROM_DRIVER
		UsbdRom_IrqHandler();
	#else
		DcdIrqHandler(1);
	#endif
#endif
}
#endif /*__LPC18XX__*/
