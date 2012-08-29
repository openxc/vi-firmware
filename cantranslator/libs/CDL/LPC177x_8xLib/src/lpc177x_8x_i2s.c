/**********************************************************************
* $Id$		lpc177x_8x_i2s.c			2011-06-02
*//**
* @file		lpc177x_8x_i2s.c
* @brief	Contains all functions support for I2S firmware library
*			on LPC177x_8x
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

/* Peripheral group ----------------------------------------------------------- */
/** @addtogroup I2S
 * @{
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc177x_8x_libcfg.h"
#else
#include "lpc177x_8x_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */
#ifdef _I2S

/* Includes ------------------------------------------------------------------- */
#include "lpc177x_8x_i2s.h"
#include "lpc177x_8x_clkpwr.h"

/* definitions ---------------------------------------------------------- */
#define    I2S_IS_ENABLED(x)   ((x)? ENABLE:DISABLE)
/* Private Functions ---------------------------------------------------------- */

static uint8_t i2s_GetWordWidth(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode);
static uint8_t i2s_GetChannel(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode);

/********************************************************************//**
 * @brief		Get I2S wordwidth value
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is the I2S mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		The wordwidth value, should be: 8,16 or 32
 *********************************************************************/
static uint8_t i2s_GetWordWidth(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode) {
	uint8_t value;

	if (TRMode == I2S_TX_MODE) {
		value = (I2Sx->DAO) & 0x03; /* get wordwidth bit */
	} else {
		value = (I2Sx->DAI) & 0x03; /* get wordwidth bit */
	}
	switch (value) {
	case I2S_WORDWIDTH_8:
		return 8;
	case I2S_WORDWIDTH_16:
		return 16;
	default:
		return 32;
	}
}

/********************************************************************//**
 * @brief		Get I2S channel value
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is the I2S mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		The channel value, should be: 1(mono) or 2(stereo)
 *********************************************************************/
static uint8_t i2s_GetChannel(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode) {
	uint8_t value;

	if (TRMode == I2S_TX_MODE) {
		value = ((I2Sx->DAO) & 0x04)>>2; /* get bit[2] */
	} else {
		value = ((I2Sx->DAI) & 0x04)>>2; /* get bit[2] */
	}
        if(value == I2S_MONO) return 1;
          return 2;
}

/* End of Private Functions --------------------------------------------------- */


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup I2S_Public_Functions
 * @{
 */

/********************************************************************//**
 * @brief		Initialize I2S
 * 					- Turn on power and clock
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @return 		none
 *********************************************************************/
void I2S_Init(LPC_I2S_TypeDef *I2Sx) {
	// Turn on power and clock
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCI2S, ENABLE);
	LPC_I2S->DAI = LPC_I2S->DAO = 0x00;
}

/********************************************************************//**
 * @brief		Configuration I2S, setting:
 * 					- master/slave mode
 * 					- wordwidth value
 * 					- channel mode
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode transmit/receive mode, should be:
 * 					- I2S_TX_MODE = 0: transmit mode
 * 					- I2S_RX_MODE = 1: receive mode
 * @param[in]	ConfigStruct pointer to I2S_CFG_Type structure
 *              which will be initialized.
 * @return 		none
 *********************************************************************/
void I2S_Config(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode, I2S_CFG_Type* ConfigStruct)
{
	uint32_t bps, config;
	/* Setup clock */
	bps = (ConfigStruct->wordwidth +1)*8;

	/* Calculate audio config */
	config = (bps - 1)<<6 | (ConfigStruct->ws_sel)<<5 | (ConfigStruct->reset)<<4 |
		(ConfigStruct->stop)<<3 | (ConfigStruct->mono)<<2 | (ConfigStruct->wordwidth);

	if(TRMode == I2S_RX_MODE){
		LPC_I2S->DAI = config;
	}else{
		LPC_I2S->DAO = config;
	}
}

/********************************************************************//**
 * @brief		DeInitial both I2S transmit or receive
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @return 		none
 *********************************************************************/
void I2S_DeInit(LPC_I2S_TypeDef *I2Sx) {
	// Turn off power and clock
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCI2S, DISABLE);
}

/********************************************************************//**
 * @brief		Get I2S Buffer Level
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode Transmit/receive mode, should be:
 * 					- I2S_TX_MODE = 0: transmit mode
 * 					- I2S_RX_MODE = 1: receive mode
 * @return 		current level of Transmit/Receive Buffer
 *********************************************************************/
uint8_t I2S_GetLevel(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode)
{

	if(TRMode == I2S_TX_MODE)
	{
		return ((I2Sx->STATE >> 16) & 0xFF);
	}
	else
	{
		return ((I2Sx->STATE >> 8) & 0xFF);
	}
}

/********************************************************************//**
 * @brief		I2S Start: clear all STOP,RESET and MUTE bit, ready to operate
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @return 		none
 *********************************************************************/
void I2S_Start(LPC_I2S_TypeDef *I2Sx)
{
	//Clear STOP,RESET and MUTE bit
	I2Sx->DAO &= ~I2S_DAI_RESET;
	I2Sx->DAI &= ~I2S_DAI_RESET;
	I2Sx->DAO &= ~I2S_DAI_STOP;
	I2Sx->DAI &= ~I2S_DAI_STOP;
	I2Sx->DAO &= ~I2S_DAI_MUTE;
}

/********************************************************************//**
 * @brief		I2S Send data
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	BufferData pointer to uint32_t is the data will be send
 * @return 		none
 *********************************************************************/
void I2S_Send(LPC_I2S_TypeDef *I2Sx, uint32_t BufferData)
{
	I2Sx->TXFIFO = BufferData;
}

/********************************************************************//**
 * @brief		I2S Receive Data
 * @param[in]	I2Sx pointer to LPC_I2S_TypeDef
 * @return 		received value
 *********************************************************************/
uint32_t I2S_Receive(LPC_I2S_TypeDef* I2Sx)
{
	return (I2Sx->RXFIFO);
}

/********************************************************************//**
 * @brief		I2S Pause
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_Pause(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode) {
	if (TRMode == I2S_TX_MODE) //Transmit mode
	{
		I2Sx->DAO |= I2S_DAO_STOP;
	} else //Receive mode
	{
		I2Sx->DAI |= I2S_DAI_STOP;
	}
}

/********************************************************************//**
 * @brief		I2S Mute
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_Mute(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode) {
	if (TRMode == I2S_TX_MODE) //Transmit mode
	{
		I2Sx->DAO |= I2S_DAO_MUTE;
	} else //Receive mode
	{
		I2Sx->DAI |= I2S_DAI_MUTE;
	}
}

/********************************************************************//**
 * @brief		I2S Stop
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_Stop(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode) {
	if (TRMode == I2S_TX_MODE) //Transmit mode
	{
		I2Sx->DAO &= ~I2S_DAO_MUTE;
		I2Sx->DAO |= I2S_DAO_STOP;
		I2Sx->DAO |= I2S_DAO_RESET;
	} else //Receive mode
	{
		I2Sx->DAI |= I2S_DAI_STOP;
		I2Sx->DAI |= I2S_DAI_RESET;
	}
}

/********************************************************************//**
 * @brief		Set frequency for I2S
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	Freq is the frequency for I2S will be set. It can range
 * 				from 16-96 kHz(16, 22.05, 32, 44.1, 48, 96kHz)
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		Status: ERROR or SUCCESS
 *********************************************************************/
Status I2S_FreqConfig(LPC_I2S_TypeDef *I2Sx, uint32_t Freq, uint8_t TRMode) {
	uint32_t i2sMclk;
	uint8_t bitrate, channel, wordwidth;

 	//use cclk/2 as default I2S reference clock
	i2sMclk = CLKPWR_GetCLK(CLKPWR_CLKTYPE_CPU)/2;
	I2Sx->TXRATE = 1  | (1<<8);
	I2Sx->RXRATE = 1  | (1<<8);
	if(TRMode == I2S_TX_MODE)
	{
		channel = i2s_GetChannel(I2Sx,I2S_TX_MODE);
		wordwidth = i2s_GetWordWidth(I2Sx,I2S_TX_MODE);
	}
	else
	{
		channel = i2s_GetChannel(I2Sx,I2S_RX_MODE);
		wordwidth = i2s_GetWordWidth(I2Sx,I2S_RX_MODE);
	}
	bitrate = i2sMclk /(Freq * channel  * wordwidth) - 1;
	if (TRMode == I2S_TX_MODE)// Transmitter
	{
		I2Sx->TXBITRATE = bitrate;
	} else //Receiver
	{
		I2Sx->RXBITRATE = bitrate;
	}
	return SUCCESS;
}

/********************************************************************//**
 * @brief		I2S set bitrate
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	bitrate value will be set
 * 				bitrate value should be in range: 0 .. 63
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_SetBitRate(LPC_I2S_TypeDef *I2Sx, uint8_t bitrate, uint8_t TRMode)
{
	if(TRMode == I2S_TX_MODE)
	{
		I2Sx->TXBITRATE = bitrate;
	}
	else
	{
		I2Sx->RXBITRATE = bitrate;
	}
}

/********************************************************************//**
 * @brief		Configuration operating mode for I2S
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	ModeConfig pointer to I2S_MODEConf_Type will be used to
 * 				configure
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_ModeConfig(LPC_I2S_TypeDef *I2Sx, I2S_MODEConf_Type* ModeConfig,
		uint8_t TRMode)
{
	if (TRMode == I2S_TX_MODE) {
		I2Sx->TXMODE &= ~0x0F; //clear bit 3:0 in I2STXMODE register
		if (ModeConfig->clksel == I2S_CLKSEL_MCLK) {
			I2Sx->TXMODE |= 0x02;
		}
		if (ModeConfig->fpin == I2S_4PIN_ENABLE) {
			I2Sx->TXMODE |= (1 << 2);
		}
		if (ModeConfig->mcena == I2S_MCLK_ENABLE) {
			I2Sx->TXMODE |= (1 << 3);
		}
	} else {
		I2Sx->RXMODE &= ~0x0F; //clear bit 3:0 in I2STXMODE register
		if (ModeConfig->clksel == I2S_CLKSEL_MCLK) {
			I2Sx->RXMODE |= 0x02;
		}
		if (ModeConfig->fpin == I2S_4PIN_ENABLE) {
			I2Sx->RXMODE |= (1 << 2);
		}
		if (ModeConfig->mcena == I2S_MCLK_ENABLE) {
			I2Sx->RXMODE |= (1 << 3);
		}
	}
}

/********************************************************************//**
 * @brief		Configure DMA operation for I2S
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	DMAConfig pointer to I2S_DMAConf_Type will be used to configure
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		none
 *********************************************************************/
void I2S_DMAConfig(LPC_I2S_TypeDef *I2Sx, I2S_DMAConf_Type* DMAConfig,
		uint8_t TRMode)
{
	if (TRMode == I2S_RX_MODE) {
		if (DMAConfig->DMAIndex == I2S_DMA_1) {
			LPC_I2S->DMA1 = (DMAConfig->depth) << 8;
		} else {
			LPC_I2S->DMA2 = (DMAConfig->depth) << 8;
		}
	} else {
		if (DMAConfig->DMAIndex == I2S_DMA_1) {
			LPC_I2S->DMA1 = (DMAConfig->depth) << 16;
		} else {
			LPC_I2S->DMA2 = (DMAConfig->depth) << 16;
		}
	}
}

/********************************************************************//**
 * @brief		Enable/Disable DMA operation for I2S
 * @param[in]	I2Sx: I2S peripheral selected, should be: LPC_I2S
 * @param[in]	DMAIndex chose what DMA is used, should be:
 * 				- I2S_DMA_1 = 0: DMA1
 * 				- I2S_DMA_2 = 1: DMA2
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @param[in]	NewState is new state of DMA operation, should be:
 * 				- ENABLE
 * 				- DISABLE
 * @return 		none
 *********************************************************************/
void I2S_DMACmd(LPC_I2S_TypeDef *I2Sx, uint8_t DMAIndex, uint8_t TRMode,
		FunctionalState NewState)
{
	if (TRMode == I2S_RX_MODE) {
		if (DMAIndex == I2S_DMA_1) {
			if (NewState == ENABLE)
				I2Sx->DMA1 |= 0x01;
			else
				I2Sx->DMA1 &= ~0x01;
		} else {
			if (NewState == ENABLE)
				I2Sx->DMA2 |= 0x01;
			else
				I2Sx->DMA2 &= ~0x01;
		}
	} else {
		if (DMAIndex == I2S_DMA_1) {
			if (NewState == ENABLE)
				I2Sx->DMA1 |= 0x02;
			else
				I2Sx->DMA1 &= ~0x02;
		} else {
			if (NewState == ENABLE)
				I2Sx->DMA2 |= 0x02;
			else
				I2Sx->DMA2 &= ~0x02;
		}
	}
}

/********************************************************************//**
 * @brief		Configure IRQ for I2S
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @param[in]	level is the FIFO level that triggers IRQ request
 * @return 		none
 *********************************************************************/
void I2S_IRQConfig(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode, uint8_t level) {
	if (TRMode == I2S_RX_MODE) {
		I2Sx->IRQ |= (level << 8);
	} else {
		I2Sx->IRQ |= (level << 16);
	}
}

/********************************************************************//**
 * @brief		Enable/Disable IRQ for I2S
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @param[in]	NewState is new state of DMA operation, should be:
 * 				- ENABLE
 * 				- DISABLE
 * @return 		none
 *********************************************************************/
void I2S_IRQCmd(LPC_I2S_TypeDef *I2Sx, uint8_t TRMode, FunctionalState NewState) {

	if (TRMode == I2S_RX_MODE) {
		if (NewState == ENABLE)
			I2Sx->IRQ |= 0x01;
		else
			I2Sx->IRQ &= ~0x01;
		//Enable DMA

	} else {
		if (NewState == ENABLE)
			I2Sx->IRQ |= 0x02;
		else
			I2Sx->IRQ &= ~0x02;
	}
}

/********************************************************************//**
 * @brief		Get I2S interrupt status
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		FunctionState	should be:
 * 				- ENABLE: interrupt is enable
 * 				- DISABLE: interrupt is disable
 *********************************************************************/
FunctionalState I2S_GetIRQStatus(LPC_I2S_TypeDef *I2Sx,uint8_t TRMode)
{

	if(TRMode == I2S_TX_MODE)
		return I2S_IS_ENABLED((I2Sx->IRQ >> 1)&0x01);
	else
		return I2S_IS_ENABLED((I2Sx->IRQ)&0x01);
}

/********************************************************************//**
 * @brief		Get I2S interrupt depth
 * @param[in]	I2Sx I2S peripheral selected, should be: LPC_I2S
 * @param[in]	TRMode is transmit/receive mode, should be:
 * 				- I2S_TX_MODE = 0: transmit mode
 * 				- I2S_RX_MODE = 1: receive mode
 * @return 		depth of FIFO level on which to create an irq request
 *********************************************************************/
uint8_t I2S_GetIRQDepth(LPC_I2S_TypeDef *I2Sx,uint8_t TRMode)
{

	if(TRMode == I2S_TX_MODE)
		return (((I2Sx->IRQ)>>16)&0xFF);
	else
		return (((I2Sx->IRQ)>>8)&0xFF);
}
/**
 * @}
 */

#endif /*_I2S*/

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

