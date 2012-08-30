#ifdef __LPC11UXX__

/**
 * @file	: lpc11Uxx_i2c.c
 * @brief	: Contains all functions support for I2C firmware library on LPC11Uxx
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
/** @addtogroup I2C
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc11uxx_i2c.h"

/* Private variables ------------------------------------------------------------------- */
/** @defgroup 	I2C_Private_Variables
 * @{
 */

/* I2C device configuration structure type */
typedef struct
{
  uint32_t      txrx_setup; 						/* Transmission setup */
  int32_t		dir;								/* Current direction phase, 0 - write, 1 - read */
  void		(*inthandler)(LPC_I2C_Type *i2c);   	/* Transmission interrupt handler */
} I2C_CFG_T;

/* I2C driver data for I2C */
static I2C_CFG_T i2cdat;


/**
 * @}
 */


/* Private functions ------------------------------------------------------------------- */
/** @defgroup 	I2C_Private_Functions
 * @{
 */


/* Generate a start condition on I2C bus (in master mode only) */
static uint32_t I2C_Start (LPC_I2C_Type *i2c);

/* Generate a stop condition on I2C bus (in master mode only) */
static void I2C_Stop (LPC_I2C_Type *i2c);

/* I2C send byte subroutine */
static uint32_t I2C_SendByte (LPC_I2C_Type *i2c, uint8_t databyte);

/* I2C get byte subroutine */
static uint32_t I2C_GetByte (LPC_I2C_Type *i2c, uint8_t *retdat, Bool ack);

/* I2C interrupt master handler */
void I2C_MasterHandler (LPC_I2C_Type *i2c);

/* I2C interrupt master handler */
void I2C_SlaveHandler (LPC_I2C_Type *i2c);

/* Enable interrupt for I2C device */
void I2C_IntCmd (LPC_I2C_Type *i2c, Bool NewState);

/*--------------------------------------------------------------------------------*/


/***********************************************************************
 * Function: I2C_Start
 * Purpose: Generate a start condition on I2C bus (in master mode only)
 * Parameters:
 *     i2cdev: Pointer to I2C register
 *     blocking: blocking or none blocking mode
 * Returns: value of I2C status register after generate a start condition
 **********************************************************************/
static uint32_t I2C_Start (LPC_I2C_Type *i2c)
{
	i2c->CONCLR = I2C_I2CONCLR_SIC;
	i2c->CONSET = I2C_I2CONSET_STA;

	// Wait for complete
	while (!(i2c->CONSET & I2C_I2CONSET_SI));
	i2c->CONCLR = I2C_I2CONCLR_STAC;
	return (i2c->STAT & I2C_STAT_CODE_BITMASK);
}


/***********************************************************************
 * Function: I2C_Stop
 * Purpose: Generate a stop condition on I2C bus (in master mode only)
 * Parameters:
 *     i2c: Pointer to I2C register
 * Returns: None
 **********************************************************************/
static void I2C_Stop (LPC_I2C_Type *i2c)
{

	/* Make sure start bit is not active */
	if (i2c->CONSET & I2C_I2CONSET_STA)
	{
		i2c->CONCLR = I2C_I2CONCLR_STAC;
	}
	i2c->CONSET = I2C_I2CONSET_STO;
	i2c->CONCLR = I2C_I2CONCLR_SIC;
}


/***********************************************************************
 * Function: I2C_SendByte
 * Purpose: Send a byte
 * Parameters:
 *     i2c: Pointer to I2C register
 * Returns: value of I2C status register after sending
 **********************************************************************/
static uint32_t I2C_SendByte (LPC_I2C_Type *i2c, uint8_t databyte)
{
	/* Make sure start bit is not active */
	if (i2c->CONSET & I2C_I2CONSET_STA)
	{
		i2c->CONCLR = I2C_I2CONCLR_STAC;
	}
	i2c->DAT = databyte & I2C_I2DAT_BITMASK;
	i2c->CONCLR = I2C_I2CONCLR_SIC;

	while (!(i2c->CONSET & I2C_I2CONSET_SI));
	return (i2c->STAT & I2C_STAT_CODE_BITMASK);
}


/***********************************************************************
 * Function: I2C_GetByte
 * Purpose: Get a byte
 * Parameters:
 *     i2c: Pointer to I2C register
 * Returns: value of I2C status register after receiving
 **********************************************************************/
static uint32_t I2C_GetByte (LPC_I2C_Type *i2c, uint8_t *retdat, Bool ack)
{
	if (ack == TRUE)
	{
		i2c->CONSET = I2C_I2CONSET_AA;
	}
	else
	{
		i2c->CONCLR = I2C_I2CONCLR_AAC;
	}
	i2c->CONCLR = I2C_I2CONCLR_SIC;

	while (!(i2c->CONSET & I2C_I2CONSET_SI));
	*retdat = (uint8_t) (i2c->DAT & I2C_I2DAT_BITMASK);
	return (i2c->STAT & I2C_STAT_CODE_BITMASK);
}



/*********************************************************************//**
 * @brief 		Enable/Disable interrupt for I2C peripheral
 * @param[in]	i2c	I2C peripheral
 * @param[in]	NewState	New State of I2C peripheral interrupt in NVIC core
 * 							should be:
 * 							- ENABLE: enable interrupt for this I2C peripheral
 * 							- DISABLE: disable interrupt for this I2C peripheral
 * @return 		None
 **********************************************************************/
void I2C_IntCmd (LPC_I2C_Type *i2c, Bool NewState)
{
	if (NewState)
	{
			NVIC_EnableIRQ(I2C_IRQn);
	}
	else
	{
			NVIC_DisableIRQ(I2C_IRQn);
	}
    return;
}


/*********************************************************************//**
 * @brief 		General Master Interrupt handler for I2C peripheral
 * @param[in]	i2c	I2C peripheral
 * @return 		None
 **********************************************************************/
void I2C_MasterHandler (LPC_I2C_Type  *i2c)
{

	uint8_t returnCode;
	I2C_M_SETUP_Type *txrx_setup;

	txrx_setup = (I2C_M_SETUP_Type *) i2cdat.txrx_setup;

	returnCode = (i2c->STAT & I2C_STAT_CODE_BITMASK);
	// Save current status
	txrx_setup->status = returnCode;
	// there's no relevant information
	if (returnCode == I2C_I2STAT_NO_INF){
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		return;
	}

	/* ----------------------------- TRANSMIT PHASE --------------------------*/
	if (i2cdat.dir == 0){
		switch (returnCode)
		{
		/* A start/repeat start condition has been transmitted -------------------*/
		case I2C_I2STAT_M_TX_START:
		case I2C_I2STAT_M_TX_RESTART:
			i2c->CONCLR = I2C_I2CONCLR_STAC;
			/*
			 * If there's any transmit data, then start to
			 * send SLA+W right now, otherwise check whether if there's
			 * any receive data for next state.
			 */
			if ((txrx_setup->tx_data != NULL) && (txrx_setup->tx_length != 0)){
				i2c->DAT = (txrx_setup->sl_addr7bit << 1);
				i2c->CONCLR = I2C_I2CONCLR_SIC;
			} else {
				goto next_stage;
			}
			break;

		/* SLA+W has been transmitted, ACK has been received ----------------------*/
		case I2C_I2STAT_M_TX_SLAW_ACK:
		/* Data has been transmitted, ACK has been received */
		case I2C_I2STAT_M_TX_DAT_ACK:
			/* Send more data */
			if ((txrx_setup->tx_count < txrx_setup->tx_length) \
					&& (txrx_setup->tx_data != NULL)){
				i2c->DAT =  *(uint8_t *)(txrx_setup->tx_data + txrx_setup->tx_count);
				txrx_setup->tx_count++;
				i2c->CONCLR = I2C_I2CONCLR_SIC;
			}
			// no more data, switch to next stage
			else {
next_stage:
				// change direction
				i2cdat.dir = 1;
				// Check if any data to receive
				if ((txrx_setup->rx_length != 0) && (txrx_setup->rx_data != NULL)){
						// check whether if we need to issue an repeat start
						if ((txrx_setup->tx_length != 0) && (txrx_setup->tx_data != NULL)){
							// Send out an repeat start command
							i2c->CONSET = I2C_I2CONSET_STA;
							i2c->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC;
						}
						// Don't need issue an repeat start, just goto send SLA+R
						else {
							goto send_slar;
						}
				}
				// no more data send, the go to end stage now
				else {
					// success, goto end stage
					txrx_setup->status |= I2C_SETUP_STATUS_DONE;
					goto end_stage;
				}
			}
			break;

		/* SLA+W has been transmitted, NACK has been received ----------------------*/
		case I2C_I2STAT_M_TX_SLAW_NACK:
		/* Data has been transmitted, NACK has been received -----------------------*/
		case I2C_I2STAT_M_TX_DAT_NACK:
			// update status
			txrx_setup->status |= I2C_SETUP_STATUS_NOACKF;
			goto retry;
		/* Arbitration lost in SLA+R/W or Data bytes -------------------------------*/
		case I2C_I2STAT_M_TX_ARB_LOST:
			// update status
			txrx_setup->status |= I2C_SETUP_STATUS_ARBF;
		default:
			goto retry;
		}
	}

	/* ----------------------------- RECEIVE PHASE --------------------------*/
	else if (i2cdat.dir == 1){
		switch (returnCode){
			/* A start/repeat start condition has been transmitted ---------------------*/
		case I2C_I2STAT_M_RX_START:
		case I2C_I2STAT_M_RX_RESTART:
			i2c->CONCLR = I2C_I2CONCLR_STAC;
			/*
			 * If there's any receive data, then start to
			 * send SLA+R right now, otherwise check whether if there's
			 * any receive data for end of state.
			 */
			if ((txrx_setup->rx_data != NULL) && (txrx_setup->rx_length != 0)){
send_slar:
				i2c->DAT = (txrx_setup->sl_addr7bit << 1) | 0x01;
				i2c->CONCLR = I2C_I2CONCLR_SIC;
			} else {
				// Success, goto end stage
				txrx_setup->status |= I2C_SETUP_STATUS_DONE;
				goto end_stage;
			}
			break;

		/* SLA+R has been transmitted, ACK has been received -----------------*/
		case I2C_I2STAT_M_RX_SLAR_ACK:
			if (txrx_setup->rx_count < (txrx_setup->rx_length - 1)) {
				/*Data will be received,  ACK will be return*/
				i2c->CONSET = I2C_I2CONSET_AA;
			}
			else {
				/*Last data will be received,  NACK will be return*/
				i2c->CONCLR = I2C_I2CONSET_AA;
			}
			i2c->CONCLR = I2C_I2CONCLR_SIC;
			break;

		/* Data has been received, ACK has been returned ----------------------*/
		case I2C_I2STAT_M_RX_DAT_ACK:
			// Note save data and increase counter first, then check later
			/* Save data  */
			if ((txrx_setup->rx_data != NULL) && (txrx_setup->rx_count < txrx_setup->rx_length)){
				*(uint8_t *)(txrx_setup->rx_data + txrx_setup->rx_count) = (i2c->DAT & I2C_I2DAT_BITMASK);
				txrx_setup->rx_count++;
			}
			if (txrx_setup->rx_count < (txrx_setup->rx_length - 1)) {
				/*Data will be received,  ACK will be return*/
				i2c->CONSET = I2C_I2CONSET_AA;
			}
			else {
				/*Last data will be received,  NACK will be return*/
				i2c->CONCLR = I2C_I2CONSET_AA;
			}

			i2c->CONCLR = I2C_I2CONCLR_SIC;
			break;

		/* Data has been received, NACK has been return -------------------------*/
		case I2C_I2STAT_M_RX_DAT_NACK:
			/* Save the last data */
			if ((txrx_setup->rx_data != NULL) && (txrx_setup->rx_count < txrx_setup->rx_length)){
				*(uint8_t *)(txrx_setup->rx_data + txrx_setup->rx_count) = (i2c->DAT & I2C_I2DAT_BITMASK);
				txrx_setup->rx_count++;
			}
			// success, go to end stage
			txrx_setup->status |= I2C_SETUP_STATUS_DONE;
			goto end_stage;

		/* SLA+R has been transmitted, NACK has been received ------------------*/
		case I2C_I2STAT_M_RX_SLAR_NACK:
			// update status
			txrx_setup->status |= I2C_SETUP_STATUS_NOACKF;
			goto retry;

		/* Arbitration lost ----------------------------------------------------*/
		case I2C_I2STAT_M_RX_ARB_LOST:
			// update status
			txrx_setup->status |= I2C_SETUP_STATUS_ARBF;
		default:
retry:
			// check if retransmission is available
			if (txrx_setup->retransmissions_count < txrx_setup->retransmissions_max){
				// Clear tx count
				txrx_setup->tx_count = 0;
				i2c->CONSET = I2C_I2CONSET_STA;
				i2c->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC;
				txrx_setup->retransmissions_count++;
			}
			// End of stage
			else {
end_stage:
				// Disable interrupt
				I2C_IntCmd(i2c, 0);
				// Send stop
				I2C_Stop(i2c);
				// Call callback if installed
				if (txrx_setup->callback != NULL){
					txrx_setup->callback();
				}
			}
			break;
		}
	}
}


/*********************************************************************//**
 * @brief 		General Slave Interrupt handler for I2C peripheral
 * @param[in]	i2c	I2C peripheral
 * @return 		None
 **********************************************************************/
void I2C_SlaveHandler (LPC_I2C_Type  *i2c)
{
	uint8_t returnCode;
	I2C_S_SETUP_Type *txrx_setup;
	uint32_t timeout;

	txrx_setup = (I2C_S_SETUP_Type *) i2cdat.txrx_setup;

	returnCode = (i2c->STAT & I2C_STAT_CODE_BITMASK);
	// Save current status
	txrx_setup->status = returnCode;
	// there's no relevant information
	if (returnCode == I2C_I2STAT_NO_INF){
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		return;
	}


	switch (returnCode)
	{

	/* No status information */
	case I2C_I2STAT_NO_INF:
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		break;

	/* Reading phase -------------------------------------------------------- */
	/* Own SLA+R has been received, ACK has been returned */
	case I2C_I2STAT_S_RX_SLAW_ACK:
	/* General call address has been received, ACK has been returned */
	case I2C_I2STAT_S_RX_GENCALL_ACK:
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		break;

	/* Previously addressed with own SLA;
	 * DATA byte has been received;
	 * ACK has been returned */
	case I2C_I2STAT_S_RX_PRE_SLA_DAT_ACK:
	/* DATA has been received, ACK hasn been return */
	case I2C_I2STAT_S_RX_PRE_GENCALL_DAT_ACK:
		/*
		 * All data bytes that over-flow the specified receive
		 * data length, just ignore them.
		 */
		if ((txrx_setup->rx_count < txrx_setup->rx_length) \
				&& (txrx_setup->rx_data != NULL)){
			*(uint8_t *)(txrx_setup->rx_data + txrx_setup->rx_count) = (uint8_t)i2c->DAT;
			txrx_setup->rx_count++;
		}
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		break;

	/* Previously addressed with own SLA;
	 * DATA byte has been received;
	 * NOT ACK has been returned */
	case I2C_I2STAT_S_RX_PRE_SLA_DAT_NACK:
	/* DATA has been received, NOT ACK has been returned */
	case I2C_I2STAT_S_RX_PRE_GENCALL_DAT_NACK:
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		break;

	/*
	 * Note that: Return code only let us know a stop condition mixed
	 * with a repeat start condition in the same code value.
	 * So we should provide a time-out. In case this is really a stop
	 * condition, this will return back after time out condition. Otherwise,
	 * next session that is slave receive data will be completed.
	 */

	/* A Stop or a repeat start condition */
	case I2C_I2STAT_S_RX_STA_STO_SLVREC_SLVTRX:
		// Temporally lock the interrupt for timeout condition
		I2C_IntCmd(i2c, 0);
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		// enable time out
		timeout = I2C_SLAVE_TIME_OUT;
		while(1){
			if (i2c->CONSET & I2C_I2CONSET_SI){
				// re-Enable interrupt
				I2C_IntCmd(i2c, 1);
				break;
			} else {
				timeout--;
				if (timeout == 0){
					// timeout occur, it's really a stop condition
					txrx_setup->status |= I2C_SETUP_STATUS_DONE;
					goto s_int_end;
				}
			}
		}
		break;

	/* Writing phase -------------------------------------------------------- */
	/* Own SLA+R has been received, ACK has been returned */
	case I2C_I2STAT_S_TX_SLAR_ACK:
	/* Data has been transmitted, ACK has been received */
	case I2C_I2STAT_S_TX_DAT_ACK:
		/*
		 * All data bytes that over-flow the specified receive
		 * data length, just ignore them.
		 */
		if ((txrx_setup->tx_count < txrx_setup->tx_length) \
				&& (txrx_setup->tx_data != NULL)){
			i2c->DAT = *(uint8_t *) (txrx_setup->tx_data + txrx_setup->tx_count);
			txrx_setup->tx_count++;
		}
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		break;

	/* Data has been transmitted, NACK has been received,
	 * that means there's no more data to send, exit now */
	/*
	 * Note: Don't wait for stop event since in slave transmit mode,
	 * since there no proof lets us know when a stop signal has been received
	 * on slave side.
	 */
	case I2C_I2STAT_S_TX_DAT_NACK:
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		txrx_setup->status |= I2C_SETUP_STATUS_DONE;
		goto s_int_end;

	// Other status must be captured
	default:
s_int_end:
		// Disable interrupt
		I2C_IntCmd(i2c, 0);
		i2c->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
		// Call callback if installed
		if (txrx_setup->callback != NULL){
			txrx_setup->callback();
		}
		break;
	}
}

/**
 * @}
 */


/* Public functions ------------------------------------------------------------------- */
/** @defgroup I2C_Public_Functions
 * @{
 */

/*********************************************************************//**
 * @brief 		Setup clock rate for I2C peripheral
 * @param[in] 	i2c	I2C peripheral
 * @param[in]	target_clock : clock of I2C (Hz)
 * @return 		None
 ***********************************************************************/
void I2C_SetClock (LPC_I2C_Type *i2c, uint32_t target_clock)
{
	uint32_t temp;

	SystemCoreClockUpdate();
	temp = SystemCoreClock  / target_clock;

	/* Set the I2C clock value to register */
	i2c->SCLH = (uint32_t)(temp / 2);
	i2c->SCLL = (uint32_t)(temp - i2c->SCLH);

	/*  I2C I/O config */
	if(target_clock<=400000)						/* Standard or Fast mode */
		temp = 0;
	else											/* Fast mode plus */
		temp = 2;
    LPC_IOCON->PIO0_4 &= ~0x3FF;
    LPC_IOCON->PIO0_4 = (0x01 | (temp<<8));			/* I2C SCL */
    LPC_IOCON->PIO0_5 &= ~0x3FF;
    LPC_IOCON->PIO0_5 = (0x01 | (temp<<8));			/* I2C SDA */

}


/*********************************************************************//**
 * @brief		De-initializes the I2C peripheral registers to their
*                  default reset values.
 * @param[in]	i2c	I2C peripheral
 * @return 		None
 **********************************************************************/
void I2C_DeInit(LPC_I2C_Type* i2c)
{

	/* Disable I2C control */
	i2c->CONCLR = I2C_I2CONCLR_I2ENC;
	/* Disable power for I2C0 module */
      LPC_SYSCON->SYSAHBCLKCTRL  =  LPC_SYSCON->SYSAHBCLKCTRL & (~(1<<5));
}


/********************************************************************//**
 * @brief		Initializes the i2c peripheral with specified parameter.
 * @param[in]	i2c	I2C peripheral
 * @param[in]	clockrate Target clock rate value to initialized I2C
 * 				peripheral
 * @return 		None
 *********************************************************************/
void I2C_Init(LPC_I2C_Type *i2c, uint32_t clockrate)
{
    LPC_SYSCON->PRESETCTRL |= (0x1<<1);
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);
    /* Set clock rate */
    I2C_SetClock(i2c, clockrate);
    /* Set I2C operation to default */
    i2c->CONCLR = (I2C_I2CONCLR_AAC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC);
}
/*********************************************************************//**
 * @brief		Enable or disable I2C peripheral's operation
 * @param[in]	i2c I2C peripheral
 * @param[in]	NewState New State of i2c peripheral's operation
 * @return 		none
 **********************************************************************/
void I2C_Cmd(LPC_I2C_Type* i2c, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		i2c->CONSET = I2C_I2CONSET_I2EN;
	}
	else
	{
		i2c->CONCLR = I2C_I2CONCLR_I2ENC;
	}
}


/*********************************************************************//**
 * @brief 		Transmit and Receive data in master mode
 * @param[in] 	i2c				Pointr to a LPC_I2C_Type structure
 * @param[in]	TransferCfg		Pointer to a I2C_M_SETUP_Type structure that
 * 								contains specified information about the
 * 								configuration for master transfer.
 * @param[in]	Opt				a I2C_TRANSFER_OPT_Type type that selected for
 * 								interrupt or polling mode.
 * @return 		SUCCESS or ERROR
 *
 * Note:
 * - In case of using I2C to transmit data only, either transmit length set to 0
 * or transmit data pointer set to NULL.
 * - In case of using I2C to receive data only, either receive length set to 0
 * or receive data pointer set to NULL.
 * - In case of using I2C to transmit followed by receive data, transmit length,
 * transmit data pointer, receive length and receive data pointer should be set
 * corresponding.
 **********************************************************************/
Status I2C_MasterTransferData(LPC_I2C_Type *i2c, I2C_M_SETUP_Type *TransferCfg, \
								I2C_TRANSFER_OPT_Type Opt)
{
	uint8_t *txdat;
	uint8_t *rxdat;
	uint32_t CodeStatus;
	uint8_t tmp;

	// reset all default state
	txdat = (uint8_t *) TransferCfg->tx_data;
	rxdat = (uint8_t *) TransferCfg->rx_data;
	// Reset I2C setup value to default state
	TransferCfg->tx_count = 0;
	TransferCfg->rx_count = 0;
	TransferCfg->status = 0;

	if (Opt == I2C_TRANSFER_POLLING){

		/* First Start condition -------------------------------------------------------------- */
		TransferCfg->retransmissions_count = 0;
retry:
		// reset all default state
		txdat = (uint8_t *) TransferCfg->tx_data;
		rxdat = (uint8_t *) TransferCfg->rx_data;
		// Reset I2C setup value to default state
		TransferCfg->tx_count = 0;
		TransferCfg->rx_count = 0;
		CodeStatus = 0;

		// Start command
		CodeStatus = I2C_Start(i2c);
		if ((CodeStatus != I2C_I2STAT_M_TX_START) \
				&& (CodeStatus != I2C_I2STAT_M_TX_RESTART)){
			TransferCfg->retransmissions_count++;
			if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
				// save status
				TransferCfg->status = CodeStatus;
				goto error;
			} else {
				goto retry;
			}
		}

		/* In case of sending data first --------------------------------------------------- */
		if ((TransferCfg->tx_length != 0) && (TransferCfg->tx_data != NULL)){

			/* Send slave address + WR direction bit = 0 ----------------------------------- */
			CodeStatus = I2C_SendByte(i2c, (TransferCfg->sl_addr7bit << 1));
			if (CodeStatus != I2C_I2STAT_M_TX_SLAW_ACK){
				TransferCfg->retransmissions_count++;
				if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
					// save status
					TransferCfg->status = CodeStatus | I2C_SETUP_STATUS_NOACKF;
					goto error;
				} else {
					goto retry;
				}
			}

			/* Send a number of data bytes ---------------------------------------- */
			while (TransferCfg->tx_count < TransferCfg->tx_length)
			{
				CodeStatus = I2C_SendByte(i2c, *txdat);
				if (CodeStatus != I2C_I2STAT_M_TX_DAT_ACK){
					TransferCfg->retransmissions_count++;
					if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
						// save status
						TransferCfg->status = CodeStatus | I2C_SETUP_STATUS_NOACKF;
						goto error;
					} else {
						goto retry;
					}
				}

				txdat++;
				TransferCfg->tx_count++;
			}
		}

		/* Second Start condition (Repeat Start) ------------------------------------------- */
		if ((TransferCfg->tx_length != 0) && (TransferCfg->tx_data != NULL) \
				&& (TransferCfg->rx_length != 0) && (TransferCfg->rx_data != NULL)){

			CodeStatus = I2C_Start(i2c);
			if ((CodeStatus != I2C_I2STAT_M_RX_START) \
					&& (CodeStatus != I2C_I2STAT_M_RX_RESTART)){
				TransferCfg->retransmissions_count++;
				if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
					// Update status
					TransferCfg->status = CodeStatus;
					goto error;
				} else {
					goto retry;
				}
			}
		}

		/* Then, start reading after sending data -------------------------------------- */
		if ((TransferCfg->rx_length != 0) && (TransferCfg->rx_data != NULL)){
			/* Send slave address + RD direction bit = 1 ----------------------------------- */

			CodeStatus = I2C_SendByte(i2c, ((TransferCfg->sl_addr7bit << 1) | 0x01));
			if (CodeStatus != I2C_I2STAT_M_RX_SLAR_ACK){
				TransferCfg->retransmissions_count++;
				if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
					// update status
					TransferCfg->status = CodeStatus | I2C_SETUP_STATUS_NOACKF;
					goto error;
				} else {
					goto retry;
				}
			}

			/* Receive a number of data bytes ------------------------------------------------- */
			while (TransferCfg->rx_count < TransferCfg->rx_length){

				/*
				 * Note that: if data length is only one, the master should not
				 * issue an ACK signal on bus after reading to avoid of next data frame
				 * on slave side
				 */
				if (TransferCfg->rx_count < (TransferCfg->rx_length - 1)){
					// Issue an ACK signal for next data frame
					CodeStatus = I2C_GetByte(i2c, &tmp, 1);
					if (CodeStatus != I2C_I2STAT_M_RX_DAT_ACK){
						TransferCfg->retransmissions_count++;
						if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
							// update status
							TransferCfg->status = CodeStatus;
							goto error;
						} else {
							goto retry;
						}
					}
				} else {
					// Do not issue an ACK signal
					CodeStatus = I2C_GetByte(i2c, &tmp, 0);
					if (CodeStatus != I2C_I2STAT_M_RX_DAT_NACK){
						TransferCfg->retransmissions_count++;
						if (TransferCfg->retransmissions_count > TransferCfg->retransmissions_max){
							// update status
							TransferCfg->status = CodeStatus;
							goto error;
						} else {
							goto retry;
						}
					}
				}
				*rxdat++ = tmp;
				TransferCfg->rx_count++;
			}
		}

		/* Send STOP condition ------------------------------------------------- */
		I2C_Stop(i2c);
		return SUCCESS;

error:
		// Send stop condition
		I2C_Stop(i2c);
		return ERROR;
	}

	else if (Opt == I2C_TRANSFER_INTERRUPT){
		// Setup tx_rx data, callback and interrupt handler
		i2cdat.txrx_setup = (uint32_t) TransferCfg;
		i2cdat.inthandler = I2C_MasterHandler;
		// Set direction phase, write first
		i2cdat.dir = 0;

		/* First Start condition -------------------------------------------------------------- */
		i2c->CONCLR = I2C_I2CONCLR_SIC;
		i2c->CONSET = I2C_I2CONSET_STA;
		I2C_IntCmd(i2c, 1);

		return (SUCCESS);
	}

	return ERROR;
}

/*********************************************************************//**
 * @brief 		Receive and Transmit data in slave mode
 * @param[in]	i2c				Pointer to a LPC_I2C_Type structure
 * @param[in]	TransferCfg		Pointer to a I2C_S_SETUP_Type structure that
 * 								contains specified information about the
 * 								configuration for master transfer.
 * @param[in]	Opt				I2C_TRANSFER_OPT_Type type that selected for
 * 								interrupt or polling mode.
 * @return 		SUCCESS or ERROR
 *
 * Note:
 * The mode of slave's operation depends on the command sent from master on
 * the I2C bus. If the master send a SLA+W command, this sub-routine will
 * use receive data length and receive data pointer. If the master send a SLA+R
 * command, this sub-routine will use transmit data length and transmit data
 * pointer.
 * If the master issue an repeat start command or a stop command, the slave will
 * enable an time out condition, during time out condition, if there's no activity
 * on I2C bus, the slave will exit, otherwise (i.e. the master send a SLA+R/W),
 * the slave then switch to relevant operation mode. The time out should be used
 * because the return status code can not show difference from stop and repeat
 * start command in slave operation.
 * In case of the expected data length from master is greater than data length
 * that slave can support:
 * - In case of reading operation (from master): slave will return I2C_I2DAT_IDLE_CHAR
 * value.
 * - In case of writing operation (from master): slave will ignore remain data from master.
 **********************************************************************/
Status I2C_SlaveTransferData(LPC_I2C_Type *i2c, I2C_S_SETUP_Type *TransferCfg, \
								I2C_TRANSFER_OPT_Type Opt)
{
	uint8_t *txdat;
	uint8_t *rxdat;
	uint32_t CodeStatus;
	uint32_t timeout;
	int32_t time_en;


	// reset all default state
	txdat = (uint8_t *) TransferCfg->tx_data;
	rxdat = (uint8_t *) TransferCfg->rx_data;
	// Reset I2C setup value to default state
	TransferCfg->tx_count = 0;
	TransferCfg->rx_count = 0;
	TransferCfg->status = 0;


	// Polling option
	if (Opt == I2C_TRANSFER_POLLING){

		/* Set AA bit to ACK command on I2C bus */
		i2c->CONSET = I2C_I2CONSET_AA;
		/* Clear SI bit to be ready ... */
		i2c->CONCLR = (I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC);

		time_en = 0;
		timeout = 0;

		while (1)
		{
			/* Check SI flag ready */
			if (i2c->CONSET & I2C_I2CONSET_SI)
			{
				time_en = 0;

				switch (CodeStatus = (i2c->STAT & I2C_STAT_CODE_BITMASK))
				{

				/* No status information */
				case I2C_I2STAT_NO_INF:
					i2c->CONSET = I2C_I2CONSET_AA;
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					break;

				/* Reading phase -------------------------------------------------------- */
				/* Own SLA+R has been received, ACK has been returned */
				case I2C_I2STAT_S_RX_SLAW_ACK:
				/* General call address has been received, ACK has been returned */
				case I2C_I2STAT_S_RX_GENCALL_ACK:
					i2c->CONSET = I2C_I2CONSET_AA;
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					break;

				/* Previously addressed with own SLA;
				 * DATA byte has been received;
				 * ACK has been returned */
				case I2C_I2STAT_S_RX_PRE_SLA_DAT_ACK:
				/* DATA has been received, ACK hasn been return */
				case I2C_I2STAT_S_RX_PRE_GENCALL_DAT_ACK:
					/*
					 * All data bytes that over-flow the specified receive
					 * data length, just ignore them.
					 */
					if ((TransferCfg->rx_count < TransferCfg->rx_length) \
							&& (TransferCfg->rx_data != NULL)){
						*rxdat++ = (uint8_t)i2c->DAT;
						TransferCfg->rx_count++;
					}
					i2c->CONSET = I2C_I2CONSET_AA;
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					break;

				/* Previously addressed with own SLA;
				 * DATA byte has been received;
				 * NOT ACK has been returned */
				case I2C_I2STAT_S_RX_PRE_SLA_DAT_NACK:
				/* DATA has been received, NOT ACK has been returned */
				case I2C_I2STAT_S_RX_PRE_GENCALL_DAT_NACK:
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					break;

				/*
				 * Note that: Return code only let us know a stop condition mixed
				 * with a repeat start condition in the same code value.
				 * So we should provide a time-out. In case this is really a stop
				 * condition, this will return back after time out condition. Otherwise,
				 * next session that is slave receive data will be completed.
				 */

				/* A Stop or a repeat start condition */
				case I2C_I2STAT_S_RX_STA_STO_SLVREC_SLVTRX:
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					// enable time out
					time_en = 1;
					timeout = 0;
					break;

				/* Writing phase -------------------------------------------------------- */
				/* Own SLA+R has been received, ACK has been returned */
				case I2C_I2STAT_S_TX_SLAR_ACK:
				/* Data has been transmitted, ACK has been received */
				case I2C_I2STAT_S_TX_DAT_ACK:
					/*
					 * All data bytes that over-flow the specified receive
					 * data length, just ignore them.
					 */
					if ((TransferCfg->tx_count < TransferCfg->tx_length) \
							&& (TransferCfg->tx_data != NULL)){
						i2c->DAT = *txdat++;
						TransferCfg->tx_count++;
					}
					i2c->CONSET = I2C_I2CONSET_AA;
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					break;

				/* Data has been transmitted, NACK has been received,
				 * that means there's no more data to send, exit now */
				/*
				 * Note: Don't wait for stop event since in slave transmit mode,
				 * since there no proof lets us know when a stop signal has been received
				 * on slave side.
				 */
				case I2C_I2STAT_S_TX_DAT_NACK:
					i2c->CONSET = I2C_I2CONSET_AA;
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					// enable time out
					time_en = 1;
					timeout = 0;
					break;

				// Other status must be captured
				default:
					i2c->CONCLR = I2C_I2CONCLR_SIC;
					goto s_error;
				}
			} else if (time_en){
				if (timeout++ > I2C_SLAVE_TIME_OUT){
					// it's really a stop condition, goto end stage
					goto s_end_stage;
				}
			}
		}

s_end_stage:
		/* Clear AA bit to disable ACK on I2C bus */
		i2c->CONCLR = I2C_I2CONCLR_AAC;
		// Check if there's no error during operation
		// Update status
		TransferCfg->status = CodeStatus | I2C_SETUP_STATUS_DONE;
		return SUCCESS;

s_error:
		/* Clear AA bit to disable ACK on I2C bus */
		i2c->CONCLR = I2C_I2CONCLR_AAC;
		// Update status
		TransferCfg->status = CodeStatus;
		return ERROR;
	}

	else if (Opt == I2C_TRANSFER_INTERRUPT){
		// Setup tx_rx data, callback and interrupt handler
		i2cdat.txrx_setup = (uint32_t) TransferCfg;
		i2cdat.inthandler = I2C_SlaveHandler;
		// Set direction phase, read first
		i2cdat.dir = 1;

		// Enable AA
		i2c->CONSET = I2C_I2CONSET_AA;
		i2c->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
		I2C_IntCmd(i2c, 1);

		return (SUCCESS);
	}

	return ERROR;
}

/*********************************************************************//**
 * @brief		Set Own slave address in I2C peripheral corresponding to
 * 				parameter specified in OwnSlaveAddrConfigStruct.
 * @param[in]	i2c	I2C peripheral selected, should be I2C
 * @param[in]	OwnSlaveAddrConfigStruct	Pointer to a I2C_OWNSLAVEADDR_CFG_Type
 * 				structure that contains the configuration information for the
*               specified I2C slave address.
 * @return 		None
 **********************************************************************/
void I2C_SetOwnSlaveAddr(LPC_I2C_Type *i2c, I2C_OWNSLAVEADDR_CFG_Type *OwnSlaveAddrConfigStruct)
{
	uint32_t tmp;

	tmp = (((uint32_t)(OwnSlaveAddrConfigStruct->SlaveAddr_7bit << 1)) \
			| ((OwnSlaveAddrConfigStruct->GeneralCallState == ENABLE) ? 0x01 : 0x00))& I2C_I2ADR_BITMASK;
	switch (OwnSlaveAddrConfigStruct->SlaveAddrChannel)
	{
	case 0:
		i2c->ADR0 = tmp;
		i2c->MASK0 = I2C_I2MASK_MASK((uint32_t) \
				(OwnSlaveAddrConfigStruct->SlaveAddrMaskValue));
		break;
	case 1:
		i2c->ADR1 = tmp;
		i2c->MASK1 = I2C_I2MASK_MASK((uint32_t) \
				(OwnSlaveAddrConfigStruct->SlaveAddrMaskValue));
		break;
	case 2:
		i2c->ADR2 = tmp;
		i2c->MASK2 = I2C_I2MASK_MASK((uint32_t) \
				(OwnSlaveAddrConfigStruct->SlaveAddrMaskValue));
		break;
	case 3:
		i2c->ADR3 = tmp;
		i2c->MASK3 = I2C_I2MASK_MASK((uint32_t) \
				(OwnSlaveAddrConfigStruct->SlaveAddrMaskValue));
		break;
	}
}


/*********************************************************************//**
 * @brief		Configures functionality in I2C monitor mode
 * @param[in]	i2c	I2C peripheral selected, should be I2C
 * @param[in]	MonitorCfgType Monitor Configuration type, should be:
 * 				- I2C_MONITOR_CFG_SCL_OUTPUT: I2C module can 'stretch'
 * 				the clock line (hold it low) until it has had time to
 * 				respond to an I2C interrupt.
 * 				- I2C_MONITOR_CFG_MATCHALL: When this bit is set to '1'
 * 				and the I2C is in monitor mode, an interrupt will be
 * 				generated on ANY address received.
 * @param[in]	NewState New State of this function, should be:
 * 				- ENABLE: Enable this function.
 * 				- DISABLE: Disable this function.
 * @return		None
 **********************************************************************/
void I2C_MonitorModeConfig(LPC_I2C_Type *i2c, uint32_t MonitorCfgType, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		i2c->MMCTRL |= MonitorCfgType;
	}
	else
	{
		i2c->MMCTRL &= (~MonitorCfgType) & I2C_I2MMCTRL_BITMASK;
	}
}


/*********************************************************************//**
 * @brief		Enable/Disable I2C monitor mode
 * @param[in]	i2c	I2C peripheral
 * @param[in]	NewState New State of this function, should be:
 * 				- ENABLE: Enable monitor mode.
 * 				- DISABLE: Disable monitor mode.
 * @return		None
 **********************************************************************/
void I2C_MonitorModeCmd(LPC_I2C_Type *i2c, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		i2c->MMCTRL |= I2C_I2MMCTRL_MM_ENA;
	}
	else
	{
		i2c->MMCTRL &= (~I2C_I2MMCTRL_MM_ENA) & I2C_I2MMCTRL_BITMASK;
	}
}


/*********************************************************************//**
 * @brief		Get data from I2C data buffer in monitor mode.
 * @param[in]	i2c	I2C peripheral
 * @return		None
 * Note:	In monitor mode, the I2C module may lose the ability to stretch
 * the clock (stall the bus) if the ENA_SCL bit is not set. This means that
 * the processor will have a limited amount of time to read the contents of
 * the data received on the bus. If the processor reads the I2DAT shift
 * register, as it ordinarily would, it could have only one bit-time to
 * respond to the interrupt before the received data is overwritten by
 * new data.
 **********************************************************************/
uint8_t I2C_MonitorGetDatabuffer(LPC_I2C_Type *i2c)
{
	return ((uint8_t)(i2c->DATA_BUFFER));
}

/*********************************************************************//**
 * @brief 		Standard Interrupt handler for I2C peripheral
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void I2C_StdIntHandler(void)
{
	i2cdat.inthandler(LPC_I2C);
}



/**
 * @}
 */

/**
 * @}
 */
/* --------------------------------- End Of File ------------------------------ */
#endif /* __LPC11UXX__ */
