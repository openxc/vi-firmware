/**********************************************************************
* $Id$		debug_frmwrk.c			2011-06-02
*//**
* @file		debug_frmwrk.c
* @brief	Contains some utilities that used for debugging through UART
* @version	1.0
* @date		02. June. 2011
* @author	NXP MCU SW Application Team
* 
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
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
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc177x_8x_libcfg.h"
#else
#include "lpc177x_8x_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */
#ifdef _DBGFWK
#include "debug_frmwrk.h"
#include "lpc177x_8x_pinsel.h"

/* Debug framework */

void (*_db_msg)(UART_ID_Type UartID, const void *s);
void (*_db_msg_)(UART_ID_Type UartID, const void *s);
void (*_db_char)(UART_ID_Type UartID, uint8_t ch);
void (*_db_dec)(UART_ID_Type UartID, uint8_t decn);
void (*_db_dec_16)(UART_ID_Type UartID, uint16_t decn);
void (*_db_dec_32)(UART_ID_Type UartID, uint32_t decn);
void (*_db_hex)(UART_ID_Type UartID, uint8_t hexn);
void (*_db_hex_16)(UART_ID_Type UartID, uint16_t hexn);
void (*_db_hex_32)(UART_ID_Type UartID, uint32_t hexn);
void (*_db_hex_)(UART_ID_Type UartID, uint8_t hexn);
void (*_db_hex_16_)(UART_ID_Type UartID, uint16_t hexn);
void (*_db_hex_32_)(UART_ID_Type UartID, uint32_t hexn);

uint8_t (*_db_get_char)(UART_ID_Type UartID);
Bool (*_db_get_char_nonblocking)(UART_ID_Type UartID, uint8_t* c);
uint8_t (*_db_get_val)(UART_ID_Type UartID, uint8_t option, uint8_t numCh, uint32_t * val);


/*********************************************************************//**
 * @brief		Puts a character to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	ch		Character to put
 * @return		None
 **********************************************************************/
void UARTPutChar (UART_ID_Type UartID, uint8_t ch)
{
	LPC_UART_TypeDef *UARTx = NULL;

	switch (UartID)
	{
		case UART_0:
			UARTx = (LPC_UART_TypeDef*)LPC_UART0;
			break;
		case UART_1:
			UARTx = (LPC_UART_TypeDef*)LPC_UART1;
			break;
		case UART_2:
			UARTx = (LPC_UART_TypeDef*)LPC_UART2;
			break;
		case UART_3:
			UARTx = (LPC_UART_TypeDef*)LPC_UART3;
			break;
		case UART_4:
			UARTx = (LPC_UART_TypeDef*)LPC_UART4;
			break;
	}
	UART_Send(UARTx, &ch, 1, BLOCKING);
}


/*********************************************************************//**
 * @brief		Get a character to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @return		character value that returned
 **********************************************************************/
uint8_t UARTGetChar (UART_ID_Type UartID)
{
	uint8_t tmp = 0;
	LPC_UART_TypeDef *UARTx = NULL;

	switch (UartID)
	{
		case UART_0:
			UARTx = (LPC_UART_TypeDef*)LPC_UART0;
			break;
		case UART_1:
			UARTx = (LPC_UART_TypeDef*)LPC_UART1;
			break;
		case UART_2:
			UARTx = (LPC_UART_TypeDef*)LPC_UART2;
			break;
		case UART_3:
			UARTx = (LPC_UART_TypeDef*)LPC_UART3;
			break;
		case UART_4:
			UARTx = (LPC_UART_TypeDef*)LPC_UART4;
			break;
	}
	UART_Receive(UARTx, &tmp, 1, BLOCKING);

	return(tmp);
}
/*********************************************************************//**
 * @brief		Get a character to UART port in non-blocking mode
 * @param[in]	     UARTx	Pointer to UART peripheral
  * @param[out]	c	character value that returned
 * @return		TRUE (there is a character for procesisng)/FALSE
 **********************************************************************/

Bool UARTGetCharInNonBlock(UART_ID_Type UartID, uint8_t* c)
{	
	LPC_UART_TypeDef *UARTx = NULL;

	switch (UartID)
	{
		case UART_0:
			UARTx = (LPC_UART_TypeDef*)LPC_UART0;
			break;
		case UART_1:
			UARTx = (LPC_UART_TypeDef*)LPC_UART1;
			break;
		case UART_2:
			UARTx = (LPC_UART_TypeDef*)LPC_UART2;
			break;
		case UART_3:
			UARTx = (LPC_UART_TypeDef*)LPC_UART3;
			break;
		case UART_4:
			UARTx = (LPC_UART_TypeDef*)LPC_UART4;
			break;
	}

    if(c == NULL)
        return FALSE;		
    if(UART_Receive(UARTx, c, 1, NONE_BLOCKING))
        return TRUE;	
    return FALSE;
}


/*********************************************************************//**
 * @brief		Get a character to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @return		character value that returned
 **********************************************************************/
uint8_t UARTGetValue (UART_ID_Type UartID, uint8_t option,
												uint8_t numCh, uint32_t* val)
{
	uint8_t tmpCh = 0, cnt, factor, isValidCh = FALSE;
	uint32_t tmpVal, rVal, cntFailed, multiplier;
	LPC_UART_TypeDef *UARTx = NULL;

	switch (UartID)
	{
		case UART_0:
			UARTx = (LPC_UART_TypeDef*)LPC_UART0;
			break;
		case UART_1:
			UARTx = (LPC_UART_TypeDef*)LPC_UART1;
			break;
		case UART_2:
			UARTx = (LPC_UART_TypeDef*)LPC_UART2;
			break;
		case UART_3:
			UARTx = (LPC_UART_TypeDef*)LPC_UART3;
			break;
		case UART_4:
			UARTx = (LPC_UART_TypeDef*)LPC_UART4;
			break;
	}
	//it does not get any value
	if(numCh <= 0)
	{
		*val = 0;
		return 0;
	}

	cntFailed = 0;

	// To store the multiplier of Decimal (10) or Heximal (16)
	factor = (option == DBG_GETVAL_IN_HEX) ? 16 : ((option == DBG_GETVAL_IN_DEC) ? 10 : 0);

	if (factor == 0)
	{
		*val = 0;

		return 0;
	}

	rVal = 0;

	while (numCh > 0)
	{
		isValidCh = TRUE;

		UART_Receive(UARTx, &tmpCh, 1, BLOCKING);

		if((tmpCh >= '0') && (tmpCh<= '9'))
		{
			tmpVal = (uint32_t) (tmpCh - '0');
		}
		else if (option == DBG_GETVAL_IN_HEX)
		{
			factor = 16;

			switch (tmpCh)
			{
				case 'a':

				case 'A':
					tmpVal = 10;

					break;

				case 'b':

				case 'B':
					tmpVal = 11;

					break;

				case 'c':

				case 'C':
					tmpVal = 12;

					break;

				case 'd':

				case 'D':
					tmpVal = 13;

					break;

				case 'e':

				case 'E':
					tmpVal = 14;

					break;

				case 'f':

				case 'F':
					tmpVal = 15;

					break;

				default:
					isValidCh = FALSE;
					break;
			}
		}
		else
		{
			isValidCh = FALSE;
		}

		multiplier = 1;

		if(isValidCh == FALSE)
		{
			if(option == DBG_GETVAL_IN_DEC)
			{
				UARTPuts(UartID, "Please enter a char from '0' to '9'!!!\r\n");
			}
			else if (option == DBG_GETVAL_IN_HEX)
			{
				UARTPuts(UartID, "Please enter a char from '0' to '9', and 'a/A', 'b/B', c/C', 'd/D', 'e/E' and 'f/F'!!!\r\n");
			}

			cntFailed ++;

			if(cntFailed >= NUM_SKIPPED_ALLOWED)
			{
				UARTPuts(UartID, "Reach limitation of re-tries. Return FAILED\r\n");

				//it's failed, should return
				return 0;
			}
		}
		else
		{
			//Echo the character to the terminal
			UARTPutChar(UartID, tmpCh);

			if(numCh == 1)
			{
				factor = 1;
				multiplier = 1;
			}
			else
			{
				for(cnt = 1; cnt < numCh; cnt++)
				{
					multiplier *= factor;
				}
			}

			tmpVal *= multiplier;

			//Update the value return
			rVal += tmpVal;

			numCh --;
		}
	}

	*val = rVal;

	return(1);
}



/*********************************************************************//**
 * @brief		Puts a string to UART port
 * @param[in]	UARTx 	Pointer to UART peripheral
 * @param[in]	str 	string to put
 * @return		None
 **********************************************************************/
void UARTPuts(UART_ID_Type UartID, const void *str)
{
	uint8_t *s = (uint8_t *) str;

	while (*s)
	{
		UARTPutChar(UartID, *s++);
	}
}


/*********************************************************************//**
 * @brief		Puts a string to UART port and print new line
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	str		String to put
 * @return		None
 **********************************************************************/
void UARTPuts_(UART_ID_Type UartID, const void *str)
{
	UARTPuts (UartID, str);
	UARTPuts (UartID, "\n\r");
}


/*********************************************************************//**
 * @brief		Puts a decimal number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	decnum	Decimal number (8-bit long)
 * @return		None
 **********************************************************************/
void UARTPutDec(UART_ID_Type UartID, uint8_t decnum)
{
	uint8_t c1=decnum%10;
	uint8_t c2=(decnum/10)%10;
	uint8_t c3=(decnum/100)%10;
	UARTPutChar(UartID, '0'+c3);
	UARTPutChar(UartID, '0'+c2);
	UARTPutChar(UartID, '0'+c1);
}

/*********************************************************************//**
 * @brief		Puts a decimal number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	decnum	Decimal number (8-bit long)
 * @return		None
 **********************************************************************/
void UARTPutDec16(UART_ID_Type UartID, uint16_t decnum)
{
	uint8_t c1=decnum%10;
	uint8_t c2=(decnum/10)%10;
	uint8_t c3=(decnum/100)%10;
	uint8_t c4=(decnum/1000)%10;
	uint8_t c5=(decnum/10000)%10;
	UARTPutChar(UartID, '0'+c5);
	UARTPutChar(UartID, '0'+c4);
	UARTPutChar(UartID, '0'+c3);
	UARTPutChar(UartID, '0'+c2);
	UARTPutChar(UartID, '0'+c1);
}

/*********************************************************************//**
 * @brief		Puts a decimal number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	decnum	Decimal number (8-bit long)
 * @return		None
 **********************************************************************/
void UARTPutDec32(UART_ID_Type UartID, uint32_t decnum)
{
	uint8_t c1=decnum%10;
	uint8_t c2=(decnum/10)%10;
	uint8_t c3=(decnum/100)%10;
	uint8_t c4=(decnum/1000)%10;
	uint8_t c5=(decnum/10000)%10;
	uint8_t c6=(decnum/100000)%10;
	uint8_t c7=(decnum/1000000)%10;
	uint8_t c8=(decnum/10000000)%10;
	uint8_t c9=(decnum/100000000)%10;
	uint8_t c10=(decnum/1000000000)%10;
	UARTPutChar(UartID, '0'+c10);
	UARTPutChar(UartID, '0'+c9);
	UARTPutChar(UartID, '0'+c8);
	UARTPutChar(UartID, '0'+c7);
	UARTPutChar(UartID, '0'+c6);
	UARTPutChar(UartID, '0'+c5);
	UARTPutChar(UartID, '0'+c4);
	UARTPutChar(UartID, '0'+c3);
	UARTPutChar(UartID, '0'+c2);
	UARTPutChar(UartID, '0'+c1);
}

/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (8-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex_ (UART_ID_Type UartID, uint8_t hexnum)
{
	uint8_t nibble, i;

	i = 1;
	do
	{
		nibble = (hexnum >> (4*i)) & 0x0F;

		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	}
	while (i--);
}


/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (8-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex (UART_ID_Type UartID, uint8_t hexnum)
{
	uint8_t nibble, i;

	UARTPuts(UartID, "0x");

	i = 1;
	do {
		nibble = (hexnum >> (4*i)) & 0x0F;
		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	} while (i--);
}


/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (16-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex16_ (UART_ID_Type UartID, uint16_t hexnum)
{
	uint8_t nibble, i;

	i = 3;
	do
	{
		nibble = (hexnum >> (4*i)) & 0x0F;

		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	}
	while (i--);
}


/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (16-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex16 (UART_ID_Type UartID, uint16_t hexnum)
{
	uint8_t nibble, i;

	UARTPuts(UartID, "0x");

	i = 3;
	do
	{
		nibble = (hexnum >> (4*i)) & 0x0F;

		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	}
	while (i--);
}

/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (32-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex32_ (UART_ID_Type UartID, uint32_t hexnum)
{
	uint8_t nibble, i;

	i = 7;
	do
	{
		nibble = (hexnum >> (4*i)) & 0x0F;

		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	}
	while (i--);
}


/*********************************************************************//**
 * @brief		Puts a hex number to UART port
 * @param[in]	UARTx	Pointer to UART peripheral
 * @param[in]	hexnum	Hex number (32-bit long)
 * @return		None
 **********************************************************************/
void UARTPutHex32 (UART_ID_Type UartID, uint32_t hexnum)
{
	uint8_t nibble, i;

	UARTPuts(UartID, "0x");

	i = 7;
	do
	{
		nibble = (hexnum >> (4*i)) & 0x0F;

		UARTPutChar(UartID, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
	}
	while (i--);
}

///*********************************************************************//**
// * @brief		print function that supports format as same as printf()
// * 				function of <stdio.h> library
// * @param[in]	None
// * @return		None
// **********************************************************************/
//void  _printf (const  char *format, ...)
//{
//    static  char  buffer[512 + 1];
//            va_list     vArgs;
//            char	*tmp;
//    va_start(vArgs, format);
//    vsprintf((char *)buffer, (char const *)format, vArgs);
//    va_end(vArgs);
//
//    _DBG(buffer);
//}

/*********************************************************************//**
 * @brief		Initialize Debug frame work through initializing UART port
 * @param[in]	None
 * @return		None
 **********************************************************************/
void debug_frmwrk_init(void)
{
	UART_CFG_Type UARTConfigStruct;

#if (USED_UART_DEBUG_PORT == 0)
	/*
	 * Initialize UART0 pin connect
	 * P0.2: TXD
	 * P0.3: RXD
	 */
	PINSEL_ConfigPin (0, 2, 1);
	PINSEL_ConfigPin (0, 3, 1);
#elif (USED_UART_DEBUG_PORT == 1)
	/*
	 * Initialize UART1 pin connect
	 * P0.15: TXD
	 * P0.16: RXD
	 */
	PINSEL_ConfigPin(0, 15, 1);
	PINSEL_ConfigPin(0, 16, 1);
#elif (USED_UART_DEBUG_PORT == 2)
	/*
	 * Initialize UART2 pin connect
	 * P0.10: TXD
	 * P0.11: RXD
	 */
	PINSEL_ConfigPin(0, 10, 1);
	PINSEL_ConfigPin(0, 11, 1);
#elif (USED_UART_DEBUG_PORT == 3)
	/*
	 * Initialize UART3 pin connect
	 * P0.2: TXD
	 * P0.3: RXD
	 */
	PINSEL_ConfigPin(0, 2, 2);
	PINSEL_ConfigPin(0, 3, 2);
#elif (USED_UART_DEBUG_PORT == 4)
	/*
	 * Initialize UART4 pin connect
	 * P0.22: TXD
	 * P2.9: RXD
	 */
	PINSEL_ConfigPin(0, 22, 3);
	PINSEL_ConfigPin(2, 9, 3);

#endif

	/* Initialize UART Configuration parameter structure to default state:
	 * Baudrate =115200 bps 
	 * 8 data bit
	 * 1 Stop bit
	 * None parity
	 */
	UART_ConfigStructInit(&UARTConfigStruct);

	// Initialize DEBUG_UART_PORT peripheral with given to corresponding parameter
	UART_Init(DEBUG_UART_PORT, &UARTConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(DEBUG_UART_PORT, ENABLE);

	_db_msg	= UARTPuts;
	_db_msg_ = UARTPuts_;
	_db_char = UARTPutChar;
	_db_hex = UARTPutHex;
	_db_hex_16 = UARTPutHex16;
	_db_hex_32 = UARTPutHex32;
	_db_hex_ = UARTPutHex_;
	_db_hex_16_ = UARTPutHex16_;
	_db_hex_32_ = UARTPutHex32_;
	_db_dec = UARTPutDec;
	_db_dec_16 = UARTPutDec16;
	_db_dec_32 = UARTPutDec32;
	_db_get_char = UARTGetChar;
	_db_get_val = UARTGetValue;
	_db_get_char_nonblocking = UARTGetCharInNonBlock;
}
#endif /*_DBGFWK*/
