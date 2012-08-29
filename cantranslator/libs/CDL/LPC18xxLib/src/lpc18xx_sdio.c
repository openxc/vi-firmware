/**********************************************************************
* $Id$		lpc18xx_sdio.c		2011-Dec-07
*//**
* @file		lpc18xx_sdio.c
* @brief	Contains all macro definitions and function prototypes
* 			support for SDIO firmware library on LPC18xx
* @version	1.0
* @date		07. Dec. 2011
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
**********************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @addtogroup SDIO
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "LPC18xx.h"                    /* LPC18xx definitions                */
#include "system_LPC18xx.h"
#include "string.h"
#include "lpc_sdmmc.h"
#include "lpc18xx_sdio.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_timer.h"

/* If this source file built with example, the LPC18xx FW library configuration
 * file in each example directory ("lpc18xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc18xx_libcfg.h"
#else
#include "lpc18xx_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */

#ifdef _SDIO

/* Interrupt Mode has not been implemented, prevent it here! */
#ifndef SDIO_USE_POLLING
#error None Polling Mode has not worked yet!
#endif

/* Global instance of the current card */
static MCI_CARD_INFO_T g_card_info;

#ifdef USE_DMADESC_DBUFF
/* Array of DMA descriptors used for double buffered mode. Must be enough to
   transfer up to 64KBytes in a single DMA session. */
static LPC_SDMMC_DMA_Type mci_dma_dd[1 + (0x10000 / (2 * MCI_DMADES1_MAXTR))];
#else
/* Array of DMA descriptors used for chained mode. Must be enough to transfer
   up to 64KBytes in a single DMA session. */
static LPC_SDMMC_DMA_Type mci_dma_dd[1 + (0x10000 / MCI_DMADES1_MAXTR)];
#endif

/* This must be defined outside the driver and is the rate of the base
   clock going into the SDIO IP */
uint32_t sdio_clk_rate;


/* Private Functions ----------------------------------------------------------- */

int sdio_execute_command(MCI_CARD_INFO_T* pdev,
                         uint32_t cmd,
                         uint32_t arg,
                         uint32_t wait_status);
/*********************************************************************//**
* @brief 		Enables the SDIO controller clock
* @param[in]	None
* @return 		None
**********************************************************************/
static void sdio_enable_clock(void)
{
	/* Enable SD MMC clock */
	CGU_ConfigPWR(CGU_PERIPHERAL_SDIO, ENABLE);
}

/*********************************************************************//**
* @brief 		Disables the SDIO controller clock
* @param[in]	None
* @return 		None
**********************************************************************/
static void sdio_disable_clock(void)
{
	/* Disable SD MMC clock */
	CGU_ConfigPWR(CGU_PERIPHERAL_SDIO, (FunctionalState)FALSE);
}

/*********************************************************************//**
* @brief 		Return clock running status
* @param[in]	None
* @return 		!0 if the clock is enabled, otherwise 0
**********************************************************************/
//int sdio_is_clock_enabled(void)
//{
//	return (LPC_CCU1->CLK_M3_SDIO_CFG & 0x1);
//}

/*********************************************************************//**
* @brief 		Setup DMA descriptors
* @param[in]	addr Address of buffer (source or destination)
* @param[in]	size size of buffer in bytes (64K max)
* @return 		None
**********************************************************************/
void sdio_dma_setup(uint32_t addr, uint32_t size)
{
	int i = 0;
	uint32_t ctrl, maxs;
#ifdef USE_DMADESC_DBUFF
	uint32_t maxs2;
#endif

	/* Reset DMA */
	LPC_SDMMC->CTRL |= MCI_CTRL_DMA_RESET | MCI_CTRL_FIFO_RESET;
 	while (LPC_SDMMC->CTRL & MCI_CTRL_DMA_RESET);

#ifdef USE_DMADESC_DBUFF
	/* Build a descriptor list using the double buffered DMA method */
	while (size > 0) {
		/* Limit size of the transfer to maximum buffer size * 2*/
		maxs = size;
		if (maxs > (MCI_DMADES1_MAXTR * 2))
			maxs = (MCI_DMADES1_MAXTR * 2);
		size -= maxs;

		/* Setup buffer sizes */
		if (maxs > MCI_DMADES1_MAXTR) {
			maxs2 = maxs - MCI_DMADES1_MAXTR;
			maxs = MCI_DMADES1_MAXTR;
		}
		else
			maxs2 = 0;

		/* Set buffer sizes */
		mci_dma_dd[i].des1 = MCI_DMADES1_BS1(maxs) | MCI_DMADES1_BS2(maxs2);

		/* Setup buffer address (double buffered) */
		mci_dma_dd[i].des2 = addr + (i * (MCI_DMADES1_MAXTR * 2));
		mci_dma_dd[i].des3 = mci_dma_dd[i].des2 + maxs;

		/* Setup basic control */
		ctrl = MCI_DMADES0_OWN; // | MCI_DMADES0_DIC;
		if (i == 0)
			ctrl |= MCI_DMADES0_FS; /* First DMA buffer */

		/* No more data? Then this is the last descriptor */
		if (!size)
			ctrl |= MCI_DMADES0_LD;
		else
			ctrl |= MCI_DMADES0_DIC;

		mci_dma_dd[i].des0 = ctrl;

		i++;
	}

	mci_dma_dd[i].des0 = MCI_DMADES0_OWN | MCI_DMADES0_LD;
	mci_dma_dd[i].des1 = 0;
	mci_dma_dd[i].des2 = 0;
	mci_dma_dd[i].des3 = 0;

#else
	/* Build a descriptor list using the chained DMA method */
	while (size > 0) {
		/* Limit size of the transfer to maximum buffer size */
		maxs = size;
		if (maxs > MCI_DMADES1_MAXTR)
			maxs = MCI_DMADES1_MAXTR;
		size -= maxs;

		/* Set buffer size */
		mci_dma_dd[i].des1 = MCI_DMADES1_BS1(maxs);

		/* Setup buffer address (chained) */
		mci_dma_dd[i].des2 = addr + (i * MCI_DMADES1_MAXTR);

		/* Setup basic control */
		ctrl = MCI_DMADES0_OWN | MCI_DMADES0_CH;
		if (i == 0)
			ctrl |= MCI_DMADES0_FS; /* First DMA buffer */

		/* No more data? Then this is the last descriptor */
		if (!size)
			ctrl |= MCI_DMADES0_LD;
		else
			ctrl |= MCI_DMADES0_DIC;

		/* Another descriptor is needed */
		mci_dma_dd[i].des3 = (uint32_t) &mci_dma_dd[i + 1];
		mci_dma_dd[i].des0 = ctrl;

		i++;
	}
#endif

	/* Set DMA derscriptor base address */
	LPC_SDMMC->DBADDR = (uint32_t) &mci_dma_dd[0];
}

/*********************************************************************//**
* @brief 		Checks whether card is acquired properly or not
* @param[in]	pdev Pointer to card info structure
* @return 		!0 if card has been acquired, otherwise 0
**********************************************************************/
static int prv_card_acquired(MCI_CARD_INFO_T* pdev)
{
  return (pdev->cid[0] != 0);
}

/*********************************************************************//**
* @brief 		Helper function to get a bit field withing multi-word
* 				buffer. Used to get fields with-in CSD & EXT-CSD
* 				structures.
* @param[in]	None
* @return 		None
**********************************************************************/
static uint32_t prv_get_bits(int start, int end, uint32_t* data)
{
  uint32_t v;
  uint32_t i = end >> 5;
  uint32_t j = start & 0x1f;

  if (i == (start >> 5))
    v = (data[i] >> j);
  else
    v = ((data[i] << (32 - j)) | (data[start >> 5] >> j));

  return (v & ((1 << (end - start + 1)) - 1));
}

/*********************************************************************//**
* @brief 		Clears the FIFOs, response and data, and the interrupt status
* @param[in]	None
* @return 		None
**********************************************************************/
static void prv_clear_all(void)
{
  /* reset all blocks */
  LPC_SDMMC->CTRL |= MCI_CTRL_FIFO_RESET;

  /* wait till resets clear */
  while (LPC_SDMMC->CTRL & MCI_CTRL_FIFO_RESET);

  /* Clear interrupt status */
  LPC_SDMMC->RINTSTS = 0xFFFFFFFF;
}

/*********************************************************************//**
* @brief 		Function to send command to Card interface unit (CIU)
* @param[in]	cmd Command with all flags set
* @param[in]	arg Argument for the command
* @return 		TRUE on times-out, otherwise FALSE
**********************************************************************/
static int prv_send_cmd(uint32_t cmd, uint32_t arg)
{
  volatile int tmo = 50;
  volatile int delay;

  /* set command arg reg*/
  LPC_SDMMC->CMDARG = arg;
  LPC_SDMMC->CMD = MCI_CMD_START | cmd;

  /* poll untill command is accepted by the CIU */
  while (--tmo && (LPC_SDMMC->CMD & MCI_CMD_START))
  {
    if (tmo & 1)
      delay = 50;
    else
      delay = 18000;

    while (--delay > 1);
  }

  return (tmo < 1) ? 1 : 0;
}

/*********************************************************************//**
* @brief 		Function to set speed of the clock going to card
* @param[in]	speed Clock speed
* @return 		TRUE on times-out, otherwise FALSE
**********************************************************************/
static void prv_set_clock(uint32_t speed)
{
  /* compute SD/MMC clock dividers */
  uint32_t div;

  div = ((sdio_clk_rate / speed) + 2) >> 1;

  if ((div == LPC_SDMMC->CLKDIV) && LPC_SDMMC->CLKENA)
    return; /* requested speed is already set */

  /* disable clock */
  LPC_SDMMC->CLKENA = 0;

  /* User divider 0 */
  LPC_SDMMC->CLKSRC = MCI_CLKSRC_CLKDIV0;

  /* inform CIU */
  prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);

  /* set divider 0 to desired value */
  LPC_SDMMC->CLKDIV = MCI_CLOCK_DIVIDER(0, div);
  /* inform CIU */
  prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);

  /* enable clock */
  LPC_SDMMC->CLKENA = MCI_CLKEN_ENABLE(0);
  /* inform CIU */
  prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);
}

/*********************************************************************//**
* @brief 		Function to retrieve command response
* @param[in]	pdev Pointer to card info structure
* @param[in]	length the length of the expected response, in bits
* @return 		None
**********************************************************************/
static void prv_pull_response(MCI_CARD_INFO_T* pdev, int length)
{
  /* on this chip response is not a fifo so read all 4 regs */
  pdev->response[0] = LPC_SDMMC->RESP0;
  pdev->response[1] = LPC_SDMMC->RESP1;
  pdev->response[2] = LPC_SDMMC->RESP2;
  pdev->response[3] = LPC_SDMMC->RESP3;
}

/*********************************************************************//**
* @brief 		Polls for command completion
* @param[in]	pdev Pointer to card info structure
* @param[in]	bit Status bits to poll for command completion
* @return 		0 on success, or failure condition (-1)
**********************************************************************/
static uint32_t prv_wait_for_completion(MCI_CARD_INFO_T* pdev, uint32_t bits)
{
  uint32_t status = 0;
  int tmo_count = US_TIMEOUT;

  /* also check error conditions */
  bits |= MCI_INT_EBE | MCI_INT_SBE | MCI_INT_HLE
          | MCI_INT_RTO | MCI_INT_RCRC | MCI_INT_RESP_ERR;

  if (bits & MCI_INT_DATA_OVER)
    bits |= MCI_INT_FRUN | MCI_INT_HTO | MCI_INT_DTO
            | MCI_INT_DCRC;

  if (pdev->wait_func == 0)
  {
    /* do busy polling when wait_func is not set*/
    do
    {
      TIM_Waitus(1);
      status = LPC_SDMMC->RINTSTS;

      if (--tmo_count < 1)
      {
        break;
      }
    }
    while ((status  & bits) == 0);
    /* set time out flag for driver timeout also */
    status |= ((tmo_count < 1) ? MCI_INT_RTO : 0);
  }
  else
  {
    /* call wait function set by application */
    status = pdev->wait_func(pdev, bits);
  }

  return status;
}

/*********************************************************************//**
* @brief 		Function to process the CSD & EXT-CSD of the card
* @param[in]	pdev Pointer to card info structure
* @return 		None
**********************************************************************/
static void prv_process_csd(MCI_CARD_INFO_T* pdev)
{
  int status = 0;
  int c_size = 0;
  int c_size_mult = 0;
  int mult = 0;

  /* compute block length based on CSD response */
  pdev->block_len = 1 << prv_get_bits(80, 83, pdev->csd);

  if ((pdev->card_type & CARD_TYPE_HC) &&
      (pdev->card_type & CARD_TYPE_SD))
  {
    /* See section 5.3.3 CSD Register (CSD Version 2.0) of SD2.0 spec
    an explanation for the calculation of these values
    */
    c_size = prv_get_bits(48, 63, (uint32_t*)pdev->csd) + 1;
    pdev->blocknr = c_size << 10; /* 512 byte blocks */
  }
  else
  {
    /* See section 5.3 of the 4.1 revision of the MMC specs for
    an explanation for the calculation of these values
    */
    c_size = prv_get_bits(62, 73, (uint32_t*)pdev->csd);
    c_size_mult = prv_get_bits(47, 49, (uint32_t*)pdev->csd); //csd_c_size_mult ();
    mult = 1 << (c_size_mult + 2);
    pdev->blocknr = (c_size + 1) * mult;
    /* adjust blocknr to 512/block */
    if (pdev->block_len > MMC_SECTOR_SIZE)
      pdev->blocknr = pdev->blocknr * (pdev->block_len >> 9);

    /* get extended CSD for newer MMC cards CSD spec >= 4.0*/
    if (((pdev->card_type & CARD_TYPE_SD) == 0) &&
        (prv_get_bits(122, 125, (uint32_t*)pdev->csd) >= 4))
    {
      /* put card in trans state */
      status = sdio_execute_command(pdev, CMD_SELECT_CARD, pdev->rca << 16, 0);
      /* set block size and byte count */
      LPC_SDMMC->BLKSIZ = MMC_SECTOR_SIZE;
      LPC_SDMMC->BYTCNT = MMC_SECTOR_SIZE;
      /* send EXT_CSD command */
	  sdio_dma_setup((uint32_t) pdev->ext_csd, MMC_SECTOR_SIZE);
      status = sdio_execute_command(pdev, CMD_SEND_EXT_CSD, 0, 0
                                   | MCI_INT_DATA_OVER);
      if ((status & MCI_INT_ERROR) == 0)
      {
        /* check EXT_CSD_VER is greater than 1.1 */
        if ((pdev->ext_csd[48] & 0xFF) > 1)
          pdev->blocknr = pdev->ext_csd[53]; /* bytes 212:215 represent sec count */

        /* switch to 52MHz clock if card type is set to 1 or else set to 26MHz */
        if ((pdev->ext_csd[49] & 0xFF) == 1)
        {
          /* for type 1 MMC cards high speed is 52MHz */
          pdev->speed = MMC_HIGH_BUS_MAX_CLOCK;
        }
        else
        {
          /* for type 0 MMC cards high speed is 26MHz */
          pdev->speed = MMC_LOW_BUS_MAX_CLOCK;
        }
      }
    }
  }

  pdev->device_size = pdev->blocknr << 9; /* blocknr * 512 */
}

/*********************************************************************//**
* @brief 		Puts current selected card in trans state
* @param[in]	pdev Pointer to card info structure
* @return 		0 on success, or error code (-1)
**********************************************************************/
static int prv_set_trans_state(MCI_CARD_INFO_T* pdev)
{
  uint32_t status;

  /* get current state of the card */
  status = sdio_execute_command(pdev, CMD_SEND_STATUS, pdev->rca << 16, 0);
  if (status & MCI_INT_RTO)
  {
    /* unable to get the card state. So return immediatly. */
    return -1;
  }
  /* check card state in response */
  status = R1_CURRENT_STATE(pdev->response[0]);
  switch (status)
  {
    case SDMMC_STBY_ST:
      /* put card in 'Trans' state */
      status = sdio_execute_command(pdev, CMD_SELECT_CARD, pdev->rca << 16, 0);
      if (status != 0)
      {
        /* unable to put the card in Trans state. So return immediatly. */
        return -1;
      }
      break;
    case SDMMC_TRAN_ST:
      /*do nothing */
      break;
    default:
      /* card shouldn't be in other states so return */
      return -1;
  }

#if SDIO_BUS_WIDTH > 1
  if (pdev->card_type & CARD_TYPE_SD)
  {
    sdio_execute_command(pdev, CMD_SD_SET_WIDTH, 2, 0);  /* SD, 4 bit width */
    /* if positive response */
    LPC_SDMMC->CTYPE = MCI_CTYPE_4BIT(0);
  }
#if SDIO_BUS_WIDTH > 4
#error 8-bit mode not supported yet!
#endif
#endif
  /* set block length */
  LPC_SDMMC->BLKSIZ = MMC_SECTOR_SIZE;
  status = sdio_execute_command(pdev, CMD_SET_BLOCKLEN, MMC_SECTOR_SIZE, 0);

  return 0;
}

/* Public Functions ----------------------------------------------------------- */
/** @addtogroup SDIO_Public_Functions
 * @{
 */

/*********************************************************************//**
* @brief 		Function to execute a command
* @param[in]	pdev Pointer to card info structure
* @param[in]	cmd Command with all flags set
* @param[in]	arg Argument for the command
* @param[in]	wait_status Status bits to poll for command completion
* @return 		0 on success, or error code (-1)
**********************************************************************/
int sdio_execute_command(MCI_CARD_INFO_T* pdev,
                         uint32_t cmd,
                         uint32_t arg,
                         uint32_t wait_status)
{
  /* if APP command there are 2 stages */
  int step = (cmd & CMD_BIT_APP) ? 2 : 1;
  int status = 0;
  uint32_t cmd_reg = 0;

  if (!wait_status)
    wait_status = (cmd & CMD_MASK_RESP) ? MCI_INT_CMD_DONE : MCI_INT_DATA_OVER;

  /* Clear the interrupts & FIFOs*/
  if (cmd & CMD_BIT_DATA)
    prv_clear_all();

  while (step)
  {
    prv_set_clock((cmd & CMD_BIT_LS) ? SD_MMC_ENUM_CLOCK : pdev->speed);

    /* Clear the interrupts */
    LPC_SDMMC->RINTSTS = 0xFFFFFFFF;

    switch (step)
    {
      case 1:     /* Execute command */
        cmd_reg = ((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD)
                  | ((cmd & CMD_BIT_INIT)  ? MCI_CMD_INIT : 0)
                  | ((cmd & CMD_BIT_DATA)  ? (MCI_CMD_DAT_EXP | MCI_CMD_PRV_DAT_WAIT) : 0)
                  | (((cmd & CMD_MASK_RESP) == CMD_RESP_R2) ? MCI_CMD_RESP_LONG : 0)
                  | ((cmd & CMD_MASK_RESP) ? MCI_CMD_RESP_EXP : 0)
                  | ((cmd & CMD_BIT_WRITE)  ? MCI_CMD_DAT_WR : 0)
                  | ((cmd & CMD_BIT_STREAM) ? MCI_CMD_STRM_MODE : 0)
                  | ((cmd & CMD_BIT_BUSY) ? MCI_CMD_STOP : 0)
                  | ((cmd & CMD_BIT_AUTO_STOP)  ? MCI_CMD_SEND_STOP : 0)
                  | MCI_CMD_START
                  ;
        /* wait for previos data finsh for select/deselect commands */
        if (((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD) == MMC_SELECT_CARD)
        {
          cmd_reg |= MCI_CMD_PRV_DAT_WAIT;
        }

        /* wait for command to be accepted by CIU */
        if (prv_send_cmd(cmd_reg, arg) == 0)
          --step;
        break;

      case 0:
        return 0;

      case 2:      /* APP prefix */
        cmd_reg = MMC_APP_CMD
                  | MCI_CMD_RESP_EXP /* Response is status */
                  | ((cmd & CMD_BIT_INIT)  ? MCI_CMD_INIT : 0)
                  | MCI_CMD_START
                  ;
        if (prv_send_cmd(cmd_reg, pdev->rca << 16) == 0)
          --step;
        break;
    }

    /* wait for command response*/
    status = prv_wait_for_completion(pdev, wait_status);

    /* We return an error if there is a timeout, even if we've fetched
    a response */
    if (status & MCI_INT_ERROR)
      return status;

    if (status & MCI_INT_CMD_DONE)
    {
      switch (cmd & CMD_MASK_RESP)
      {
        case 0:
          break;
        case CMD_RESP_R1:
        case CMD_RESP_R3:
          prv_pull_response(pdev, 48);
          break;
        case CMD_RESP_R2:
          prv_pull_response(pdev, 136);
          break;
      }
    }
  }
  return 0;
}

/*********************************************************************//**
* @brief 		Function to enumerate the SD/MMC/SDHC/MMC+ cards
* @param[in]	None
* @return 		1 if a card is acquired, otherwise 0
**********************************************************************/
int sdio_acquire(void)
{
  MCI_CARD_INFO_T* pdev = &g_card_info;
  int status;
  int tries = 0;
  uint32_t ocr = OCR_VOLTAGE_RANGE_MSK;
  uint32_t r;
  int state = 0;
  uint32_t command = 0;
  /* preserve wait_func and irq callback*/
  MCI_CMD_WAIT_FUNC_T temp = pdev->wait_func;
  MCI_IRQ_CB_FUNC_T tempirq = pdev->irq_callback;

  /* clear card struct */
  memset(pdev, 0, sizeof(MCI_CARD_INFO_T));
  /* restore wait_func */
  pdev->wait_func = temp;
  pdev->irq_callback = tempirq;

  /* clear card type */
  LPC_SDMMC->CTYPE = 0;

  /* set high speed for the card as 20MHz */
  pdev->speed = MMC_MAX_CLOCK;

  status = sdio_execute_command(pdev, CMD_IDLE, 0, MCI_INT_CMD_DONE);

  while (state < 100)
  {
    switch (state)
    {
      case 0:     /* Setup for SD */
        /* check if it is SDHC card */
        status = sdio_execute_command(pdev, CMD_SD_SEND_IF_COND, SD_SEND_IF_ARG, 0);
        if (!(status & MCI_INT_RTO))
        {

          /* check response has same echo pattern */
          if ((pdev->response[0] & SD_SEND_IF_ECHO_MSK) == SD_SEND_IF_RESP)
            /* it is SD 2.0 card so indicate we are SDHC capable*/
            ocr |= OCR_HC_CCS;
        }

        ++state;
        command = CMD_SD_OP_COND;
        tries = INIT_OP_RETRIES;
        /* assume SD card */
        pdev->card_type |= CARD_TYPE_SD;
        /* for SD cards high speed is 25MHz */
        pdev->speed = SD_MAX_CLOCK;

        break;

      case 10:      /* Setup for MMC */
        /* start fresh for MMC crds */
        pdev->card_type &= ~CARD_TYPE_SD;
        status = sdio_execute_command(pdev, CMD_IDLE, 0, MCI_INT_CMD_DONE);
        command = CMD_MMC_OP_COND;
        tries = INIT_OP_RETRIES;
        ocr |= OCR_HC_CCS;
        ++state;
        /* for MMC cards high speed is 20MHz */
        pdev->speed = MMC_MAX_CLOCK;
        break;

      case 1:
      case 11:
        status = sdio_execute_command(pdev, command, 0, 0);
        if (status & MCI_INT_RTO)
          state += 9;		/* Mode unavailable */
        else
          ++state;
        break;

      case 2:			/* Initial OCR check  */
      case 12:
        ocr = pdev->response[0] | (ocr & OCR_HC_CCS);
        if (ocr & OCR_ALL_READY)
          ++state;
        else
          state += 2;
        break;

      case 3:			/* Initial wait for OCR clear */
      case 13:
        while ((ocr & OCR_ALL_READY) && --tries > 0)
        {
          TIM_Waitms(MS_ACQUIRE_DELAY);
          status = sdio_execute_command(pdev, command, 0, 0);
          ocr = pdev->response[0] | (ocr & OCR_HC_CCS);
        }
        if (ocr & OCR_ALL_READY)
          state += 7;
        else
          ++state;
        break;

      case 14:
        /* for MMC cards set high capacity bit */
        ocr |= OCR_HC_CCS;
      case 4:     /* Assign OCR */
        tries = SET_OP_RETRIES;
        ocr &= OCR_VOLTAGE_RANGE_MSK | OCR_HC_CCS;	/* Mask for the bits we care about */
        do
        {
          TIM_Waitms(MS_ACQUIRE_DELAY);
          status = sdio_execute_command(pdev, command, ocr, 0);
          r = pdev->response[0];
        }
        while (!(r & OCR_ALL_READY) && --tries > 0);
        if (r & OCR_ALL_READY)
        {
          /* is it high capacity card */
          pdev->card_type |= (r & OCR_HC_CCS);
          ++state;
        }
        else
          state += 6;

        break;

      case 5:     /* CID polling */
      case 15:
        status = sdio_execute_command(pdev, CMD_ALL_SEND_CID, 0, 0);
        memcpy(pdev->cid, pdev->response, 16);
        ++state;
        break;

      case 6:     /* RCA send, for SD get RCA */
        status = sdio_execute_command(pdev, CMD_SD_SEND_RCA, 0, 0);
        pdev->rca = (pdev->response[0]) >> 16;
        ++state;
        break;
      case 16:      /* RCA assignment for MMC set to 1 */
        pdev->rca = 1;
        status = sdio_execute_command(pdev, CMD_MMC_SET_RCA, pdev->rca << 16, 0);
        ++state;
        break;

      case 7:
      case 17:
        status = sdio_execute_command(pdev, CMD_SEND_CSD, pdev->rca << 16, 0);
        memcpy(pdev->csd, pdev->response, 16);
        state = 100;
        break;

      default:
        state += 100; /* break from while loop */
        break;
    }
  }

  /* Compute card size, block size and no. of blocks
     based on CSD response recived. */
  if (prv_card_acquired(pdev))
    prv_process_csd(pdev);

  return prv_card_acquired(&g_card_info);
}

/*********************************************************************//**
* @brief 		Initializes the MCI card controller
* @param[in]	waitfunc Pointer to wait function to be used during for poll command status
* @param[in]	irqfunc Pointer to IRQ status callback
* @return 		None
**********************************************************************/
void sdio_init(uint32_t sdio_clock)
{
  volatile uint32_t i;
	
  sdio_clk_rate = sdio_clock;
  /* enable SD/MMC clock */
  sdio_enable_clock();

  /* Software reset */
  LPC_SDMMC->BMOD = MCI_BMOD_SWR;

  /* reset all blocks */
  LPC_SDMMC->CTRL = MCI_CTRL_RESET | MCI_CTRL_FIFO_RESET
              | MCI_CTRL_DMA_RESET;
  while (LPC_SDMMC->CTRL &
         (MCI_CTRL_RESET | MCI_CTRL_FIFO_RESET | MCI_CTRL_DMA_RESET));

  /* Internal DMA setup for control register */
#ifdef SDIO_USE_POLLING
  LPC_SDMMC->CTRL = MCI_CTRL_USE_INT_DMAC;
#else
  LPC_SDMMC->CTRL = MCI_CTRL_USE_INT_DMAC | MCI_CTRL_INT_ENABLE;
  LPC_SDMMC->INTMASK = 0xFF<<16 | 0xFFFF;
#endif
  /* Clear the interrupts for the host controller */
  LPC_SDMMC->RINTSTS = 0xFFFFFFFF;

  /* Put in max timeout */
  LPC_SDMMC->TMOUT = 0xFFFFFFFF;

  /* FIFO threshold settings for DMA, DMA burst of 4,
     FIFO watermark at 16 */
  LPC_SDMMC->FIFOTH = (0x1 << 28) | (0xF << 16) | (0x10 << 0);

  /* Enable DMA, burst size of 4, fixed burst */
  LPC_SDMMC->BMOD = MCI_BMOD_DE | MCI_BMOD_PBL4 | MCI_BMOD_DSL(4);

  /* disable clock to CIU (needs latch) */
  LPC_SDMMC->CLKENA = 0;
  LPC_SDMMC->CLKSRC = 0;

  /* clear mmc structure*/
  memset(&g_card_info, 0, sizeof(MCI_CARD_INFO_T));
  /* set the wait_func if passed */
  g_card_info.wait_func = NULL;
  g_card_info.irq_callback = NULL;

	/* If not in polling mode, enable SDIO IRQ */
#ifndef SDIO_USE_POLLING
	NVIC_EnableIRQ(SDIO_IRQn);
#endif
}

/*********************************************************************//**
* @brief 		Detect if an SD card is inserted
* @param[in]	None
* @return 		Returns 1 if a card is detected, otherwise 0
**********************************************************************/
int sdio_card_detect(void)
{
	/* No card = high state in regsiter */
	if (LPC_SDMMC->CDETECT & 1)
		return 0;

	return 1;
}

/*********************************************************************//**
* @brief 		Detect if write protect is enabled
* @param[in]	None
* @return 		Returns 1 if card is write protected, otherwise 0
**********************************************************************/
int sdio_card_wp_on(void)
{
	if (LPC_SDMMC->WRTPRT & 1)
		return 1;

	return 0;
}

/*********************************************************************//**
* @brief 		Enable or disable slot power
* @param[in]	enable !0 to enable, or 0 to disable
* @return 		None
**********************************************************************/
void sdio_power_onoff(int enable)
{
	if (enable)
		LPC_SDMMC->PWREN = 1;
	else
		LPC_SDMMC->PWREN = 0;
}

/*********************************************************************//**
* @brief 		Get card's current state (idle, transfer, program, etc.)
* @param[in]	None
* @return 		Current transfer state (0 -
**********************************************************************/
int sdio_get_state(void)
{
  uint32_t status;

  /* get current state of the card */
  status = sdio_execute_command(&g_card_info, CMD_SEND_STATUS, g_card_info.rca << 16, 0);
  if (status & MCI_INT_RTO)
  {
    /* unable to get the card state. So return immediatly. */
    return -1;
  }
  /* check card state in response */
  return (int) R1_CURRENT_STATE(g_card_info.response[0]);
}

/*********************************************************************//**
* @brief 		Close the MCI
* @param[in]	None
* @return 		None
**********************************************************************/
void sdio_deinit(void)
{
  /* Place card in idle state */
  if (prv_card_acquired(&g_card_info) == 0)
    sdio_execute_command(&g_card_info, CMD_IDLE, 0, MCI_INT_CMD_DONE);

  /* clear mmc structure*/
  memset(&g_card_info, 0, sizeof(MCI_CARD_INFO_T));
  sdio_disable_clock();
}

/*********************************************************************//**
* @brief 		Performs the read of data from the SD/MMC card
* @param[in]	buffer    Pointer to data buffer to copy to
* @param[in]  	start_block Start block number
* @param[in]  	end_block End block number
* @return 		Bytes read, or 0 on error
**********************************************************************/
int sdio_read_blocks(void* buffer,
                     int start_block,
                     int end_block)
{
  MCI_CARD_INFO_T* pdev = &g_card_info;
  int cbRead = (end_block - start_block + 1) << 9; /*(end_block - start_block) * 512 */
  int status = 0;
  int index;

  /* if card is not acquired return immediately */
  if ((prv_card_acquired(pdev) == 0)
      || (end_block < start_block) /* check block index in range */
      || (start_block < 0)
      || (end_block > pdev->blocknr)
     )
  {
    return 0;
  }
  /* put card in trans state */
  if (prv_set_trans_state(pdev) != 0)
    return 0;

  /* set number of bytes to read */
  LPC_SDMMC->BYTCNT = cbRead;

  /* if high capacity card use block indexing */
  if (pdev->card_type & CARD_TYPE_HC)
    index = start_block;
  else
    index = start_block << 9;

  sdio_dma_setup((uint32_t) buffer, cbRead);

  /* check how many blocks to read */
  if (end_block ==  start_block)
  {
    status = sdio_execute_command(pdev, CMD_READ_SINGLE, index,
                                 0 | MCI_INT_DATA_OVER);
  }
  else
  {
    /* do read multiple */
    status = sdio_execute_command(pdev, CMD_READ_MULTIPLE, index,
                                 0 | MCI_INT_DATA_OVER);
  }

  if (status != 0)
    cbRead = 0; /* return error if command fails */

  return cbRead;
}

/*********************************************************************//**
* @brief 		Performs write of data to the SD/MMC card
* @param[in]	buffer    Pointer to data buffer to copy to
* @param[in]  	start_block Start block number
* @param[in]  	end_block End block number
* @return 		Number of bytes actually written, or 0 on error
**********************************************************************/
int sdio_write_blocks(void* buffer,
                      int start_block,
                      int end_block)
{
  MCI_CARD_INFO_T* pdev = &g_card_info;
  /*(end_block - start_block) * 512 */
  int cbWrote = (end_block - start_block + 1) << 9;
  int status;
  int index;

  /* if card is not acquired return immediately */
  if ((prv_card_acquired(pdev) == 0)
      || (end_block < start_block) /* check block index in range */
      || (start_block < 0)
      || (end_block > pdev->blocknr)
     )
  {
    return 0;
  }

  /* put card in trans state */
  if (prv_set_trans_state(pdev) != 0)
    return 0;

  /* set number of bytes to write */
  LPC_SDMMC->BYTCNT = cbWrote;

  /* if high capacity card use block indexing */
  if (pdev->card_type & CARD_TYPE_HC)
    index = start_block;
  else
    index = start_block << 9;

  sdio_dma_setup((uint32_t) buffer, cbWrote);

  /* check how many blocks to write */
  if (end_block == start_block)
  {
    status = sdio_execute_command(pdev, CMD_WRITE_SINGLE, index,
                                 0 | MCI_INT_DATA_OVER);
  }
  else
  {
    /* do write multiple */
    status = sdio_execute_command(pdev, CMD_WRITE_MULTIPLE, index,
                                 0 | MCI_INT_DATA_OVER);
  }

  if (status != 0)
	cbWrote = 0;

  return cbWrote;
}

/*********************************************************************//**
* @brief 		SDIO controller interrupt handler
* @param[in]	None
* @return 		None
**********************************************************************/
void SDIO_IRQHandler(void)
{
	if (g_card_info.irq_callback)
		g_card_info.irq_callback(&g_card_info, LPC_SDMMC->RINTSTS);
	LPC_SDMMC->RINTSTS = LPC_SDMMC->RINTSTS;
}
/**
 * @}
 */

#endif /* _SDIO */

/**
 * @}
 */
