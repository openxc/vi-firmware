#ifdef __LPC11UXX__

/**
 * @file	: lpc11Uxx_pinsel.c
 * @brief	: Contains all functions support for Pin connect block firmware
 * 				library on LPC11Uxx
 * @version	:
 * @date	:
 * @author	:
 **************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **********************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @addtogroup PINSEL
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc11uxx_pinsel.h"

/* Private Functions ---------------------------------------------------------- */

/*********************************************************************//**
 * @brief		Get pointer to GPIO peripheral due to GPIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 1.
 * @return		Pointer to GPIO peripheral
 **********************************************************************/
uint32_t *PIN_GetPointer(uint8_t portnum, uint8_t pinnum)
{
	uint32_t *pPIN = NULL;

	if(((portnum == PINSEL_PORT_0)&(pinnum>PINSEL_PIN_23)))
		return NULL;

	if(portnum == PINSEL_PORT_0)
	{
		pPIN = (uint32_t*)(LPC_IOCON_BASE + pinnum*4);
	}
	else if(portnum == PINSEL_PORT_1)
	{
		pPIN = (uint32_t*)(LPC_IOCON_BASE + 0x00000060 + pinnum*4);
	}
	return pPIN;
}

/* Public Functions ----------------------------------------------------------- */
/** @addtogroup PINSEL_Public_Functions
 * @{
 */

/*********************************************************************//**
 * @brief 		Setup the pin selection function
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 *
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
 *				- PINSEL_PIN_0 : Pin 0
 *				- PINSEL_PIN_1 : Pin 1
 *				.....
 * @param[in] 	funcnum Function number,
 * 				should be one of the following:
 *				- PINSEL_FUNC_0 : default function
 *				- PINSEL_FUNC_1 : first alternate function
 *				- PINSEL_FUNC_2 : second alternate function
 *				- PINSEL_FUNC_3 : third alternate function
 *
 * @return 		None
 **********************************************************************/
void PINSEL_SetPinFunc ( uint8_t portnum, uint8_t pinnum, uint8_t funcnum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	if(pPIN!=NULL)
	{
		*pPIN &= ~0x00000007;					//Clear function bits
		*pPIN |= funcnum;
	}
}

/*********************************************************************//**
 * @brief 		Setup resistor mode for each pin
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
 *				- PINSEL_PIN_0 : Pin 0
 *				- PINSEL_PIN_1 : Pin 1
 *				.....
 * @param[in] 	modenum: Mode number,
 * 				should be one of the following:
 *				- PINSEL_PINMODE_INACTIVE	: No pull-up or pull-down resistor
 *				- PINSEL_PINMODE_PULLUP		: Internal pull-up resistor
 *				- PINSEL_PINMODE_DOWN		: Internal pull-up resistor
 *				- PINSEL_PINMODE_REPEATER
 * @return 		None
 **********************************************************************/
void PINSEL_SetPinMode ( uint8_t portnum, uint8_t pinnum, uint8_t modenum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	if(pPIN!=NULL)
	{
		*pPIN &= ~0x00000018;				//Clear function bits
		*pPIN |= (modenum << 3);
	}
}
/*********************************************************************//**
 * @brief 		Setup hysteresis for each pin
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
 *				- PINSEL_PIN_0 : Pin 0
 *				- PINSEL_PIN_1 : Pin 1
 *				.....
 * @param[in] 	hysnum: Mode number,
 * 				should be one of the following:
 *				- PINSEL_PINHYS_DISABLE : Disable
 *				- PINSEL_PINHYS_ENABLE : Enable
 * @return 		None
 **********************************************************************/
void PINSEL_SetHysMode ( uint8_t portnum, uint8_t pinnum, uint8_t hysnum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	if(pPIN==NULL) return;
	if(hysnum == PINSEL_PINHYS_DISABLE)
	{
		*pPIN &= ~(0x01 << 5);						//Clear hys bits
	}
	else if(hysnum == PINSEL_PINHYS_ENABLE)
	{
		*pPIN |= (0x01 << 5);
	}
}

/*********************************************************************//**
 * @brief 		Configure Pin corresponding to specified parameters passed
 * 				in the PinCfg
 * @param[in]	PinCfg	Pointer to a PINSEL_CFG_Type structure
 *                    that contains the configuration information for the
 *                    specified pin.
 * @return 		None
 **********************************************************************/
void PINSEL_ConfigPin(PINSEL_CFG_Type *PinCfg)
{
	PINSEL_SetPinFunc(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Funcnum);
	PINSEL_SetPinMode(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Funcmode);
	PINSEL_SetHysMode(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Hystereris);
}


/**
 * @}
 */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
#endif /* __LPC11UXX__ */
