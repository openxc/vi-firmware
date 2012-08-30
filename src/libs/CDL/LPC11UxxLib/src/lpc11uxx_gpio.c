#ifdef __LPC11UXX__

/**
 * @file	: lpc11Uxx_gpio.c
 * @brief	: Contains all functions support for GPIO firmware library on LPC11Uxx
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
/** @addtogroup GPIO
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc11uxx_gpio.h"

/* Private Functions ---------------------------------------------------------- */


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup GPIO_Public_Functions
 * @{
 */


/* GPIO ------------------------------------------------------------------------------ */

/*********************************************************************//**
 * @brief		Set Direction for GPIO port.
 * @param[in]	portNum		Port Number value, should be in range from 0 to 1
 * @param[in]	bitValue	Value that contains all bits to set direction,
 * 							in range from 0 to 0xFFFFFFFF.
 * 							example: value 0x5 to set out direction for bit 0 and bit 2.
 * @param[in]	dir			Direction value, should be:
 * 							- 0: Input.
 * 							- 1: Output.
 * @return		None
 *
 * Note: All remaining bits that are not activated in bitValue (value '0')
 * will not be effected by this function.
 **********************************************************************/
void GPIO_SetDir(uint8_t portNum, uint32_t bitValue, uint8_t dir)
{
	if(portNum<=PORT1)
	{
		// Enable Output
		if (dir) {
			LPC_GPIO->DIR[portNum] |= bitValue;
		}
		// Enable Input
		else {
			LPC_GPIO->DIR[portNum] &= ~bitValue;
		}
	}
}


/*********************************************************************//**
 * @brief		Set Value for bits that have output direction on GPIO port.
 * @param[in]	portNum		Port number value, should be in range from 0 to 1
 * @param[in]	bitValue	Value that contains all bits on GPIO to set,
 * 							in range from 0 to 0xFFFFFFFF.
 * 							example: value 0x5 to set bit 0 and bit 2.
 * @return		None
 *
 * Note:
 * - For all bits that has been set as input direction, this function will
 * not effect.
 * - For all remaining bits that are not activated in bitValue (value '0')
 * will not be effected by this function.
 **********************************************************************/
void GPIO_SetValue(uint8_t portNum, uint32_t bitValue)
{
	if(portNum<=PORT1)
	{
		LPC_GPIO->PIN[portNum] |= bitValue;
	}
}
/*********************************************************************//**
 * @brief		Clear Value for bits that have output direction on GPIO port.
 * @param[in]	portNum		Port number value, should be in range from 0 to 1
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 0xFFFFFFFF.
 * 							example: value 0x5 to clear bit 0 and bit 2.
 * @return		None
 *
 * Note:
 * - For all bits that has been set as input direction, this function will
 * not effect.
 * - For all remaining bits that are not activated in bitValue (value '0')
 * will not be effected by this function.
 **********************************************************************/
void GPIO_ClearValue(uint8_t portNum, uint32_t bitValue)
{
	if(portNum<=PORT1)
	{
		LPC_GPIO->PIN[portNum] &= ~bitValue;
	}
}

/*********************************************************************//**
 * @brief		Read Current state on port pin that have input direction of GPIO
 * @param[in]	portNum		Port number to read value, in range from 0 to 1
 * @return		Current value of GPIO port.
 *
 * Note: Return value contain state of each port pin (bit) on that GPIO regardless
 * its direction is input or output.
 **********************************************************************/
uint32_t GPIO_ReadValue(uint8_t portNum)
{
	if(portNum<=PORT1)
	{
		return LPC_GPIO->PIN[portNum];
	}
	return (0);
}

/**
 * @}
 */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
#endif /* __LPC11UXX__ */
