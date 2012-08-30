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


#include "../bsp_internal.h"

#if (BOARD == BOARD_HITEX_A4)
#include <string.h>
void bsp_init(void)
{
	uint32_t Core_Clock;
	/* Set up core clock */
	CGU_Init();

	/* Initialize the external memory controller */
	vEMC_InitSRDRAM(SDRAM_BASE_ADDR, SDRAM_WIDTH, SDRAM_SIZE_MBITS, SDRAM_DATA_BUS_BITS, SDRAM_COL_ADDR_BITS);

	/* Turn on 5V USB VBUS TODO Should be Host-only */
	scu_pinmux(0x9, 5, MD_PUP | MD_EZI, FUNC2);		// P9_5 USB1_VBUS_EN, USB1 VBus function
	scu_pinmux(0x2, 5, MD_PLN | MD_EZI | MD_ZI, FUNC2);		// P2_5 USB1_VBUS, MUST CONFIGURE THIS SIGNAL FOR USB1 NORMAL OPERATION

#if (BOARD == BOARD_HITEX_A4)
	scu_pinmux(0x6, 3, MD_PUP | MD_EZI, FUNC1);		// P6_3 USB0_PWR_EN, USB0 VBus function
#else
	scu_pinmux(0x2, 3, MD_PUP | MD_EZI, FUNC7);		// USB0 VBus function
#endif
}

void Serial_Init(const uint32_t BaudRate, const bool DoubleSpeed)
{
	UART_CFG_Type UARTConfigStruct;

	/*
	 * Initialize UART1 pin connect
	 */
#if (BOARD == BOARD_HITEX_A4)
	scu_pinmux(0xC ,13 , MD_PDN | MD_EZI, FUNC2); 	// PC.13 : UART1_TXD
	scu_pinmux(0xC ,14 , MD_PDN | MD_EZI, FUNC2); 	// PC.14 : UART1_RXD
#else
	scu_pinmux(0xF ,10 , MD_PDN | MD_EZI, FUNC1); 	// UART0_TXD
	scu_pinmux(0xF ,11 , MD_PDN | MD_EZI, FUNC1); 	// UART0_RXD
#endif

	/* Initialize UART Configuration parameter structure to default state:
	 * Baudrate = 9600bps
	 * 8 data bit
	 * 1 Stop bit
	 * None parity
	 */
	UART_ConfigStructInit(&UARTConfigStruct);
	UARTConfigStruct.Baud_rate = BaudRate;	// Re-configure baudrate

	// Initialize DEBUG_UART_PORT peripheral with given to corresponding parameter
	UART_Init(DEBUG_UART_PORT, &UARTConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(DEBUG_UART_PORT, ENABLE);
}

void Serial_Send(uint8_t* buffer, uint32_t size, TRANSFER_BLOCK_Type block_config)
{
	UART_Send(DEBUG_UART_PORT, (uint8_t*) buffer, size, block_config);
}

uint32_t Serial_Revc(uint8_t* buffer, uint32_t size, TRANSFER_BLOCK_Type block_config)
{
	return UART_Receive(DEBUG_UART_PORT, (uint8_t*) buffer, size, block_config);
}

#define LPC_I2C LPC_I2C0
void Buttons_Init(void)	// FIXME not functional ATM
{
	I2C_M_SETUP_Type transferMCfg;
	uint8_t sendbuf[4]; //,receivebuf,tem;

	sendbuf[0] = PCA9502_REG_ADDR(PCA9502_REG_IOINTENA);
	sendbuf[1] = 0xF0;						/* enable interrupt for IO[7:4] */
	transferMCfg.sl_addr7bit = I2CDEV_PCA9502_ADDR;
	transferMCfg.tx_data = sendbuf ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);

	/* Configure GPIO_IRQ pin */
#if (BOARD == BOARD_HITEX_A4)
	scu_pinmux(0xA, 1, MD_PUP | MD_EZI, FUNC0);			// PA_1  pullup, func0 --> GPIO4[8]
#else
	scu_pinmux(0xD, 7, MD_PUP | MD_EZI, FUNC4);			// GPIO6[21]
#endif

	GPIO_SetDir(BUTTONS_BUTTON1_GPIO_PORT_NUM, (1<<BUTTONS_BUTTON1_GPIO_BIT_NUM), 0); // input
}

void LEDs_Init(void)
{
	I2C_M_SETUP_Type transferMCfg;
	uint8_t sendbuf[4]; //,receivebuf,tem;

	/* Initiate I2C */
	I2C_Init(LPC_I2C, 100000);
	I2C_Cmd(LPC_I2C, ENABLE);

	/* Configure PCA9502 */
	sendbuf[0] = PCA9502_REG_ADDR(PCA9502_REG_IODIR);
	sendbuf[1] = 0x0F;						/* IO[7:4]=input IO[3:0]=output */
	transferMCfg.sl_addr7bit = I2CDEV_PCA9502_ADDR;
	transferMCfg.tx_data = sendbuf ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);

	sendbuf[0] = PCA9502_REG_ADDR(PCA9502_REG_IOSTATE);
	sendbuf[1] = 0x0F;						/* all output low */
	transferMCfg.sl_addr7bit = I2CDEV_PCA9502_ADDR;
	transferMCfg.tx_data = sendbuf ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);
}

void LEDs_SetAllLEDs(uint32_t LEDMask)
{
	I2C_M_SETUP_Type transferMCfg;
	uint8_t sendbuf[2];

	sendbuf[0] = PCA9502_REG_ADDR(PCA9502_REG_IOSTATE);
	sendbuf[1] = ~(uint8_t)(LEDMask & 0x0F);
	transferMCfg.sl_addr7bit = I2CDEV_PCA9502_ADDR;
	transferMCfg.tx_data = sendbuf ;
	transferMCfg.tx_length = 2;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);
}

/**
 * UDA1380 definition
 */
/* UDA1380 Register Address */
typedef enum {
  UDA_EVALM_CLK = 0x00,
  UDA_BUS_CTRL,
  UDA_POWER_CTRL,
  UDA_ANALOG_CTRL,
  UDA_HPAMP_CTRL,
  UDA_MASTER_VOL_CTRL = 0x10,
  UDA_MIXER_VOL_CTRL,
  UDA_MODE_CTRL,
  UDA_MUTE_CTRL,
  UDA_MIXER_FILTER_CTRL,
  UDA_DEC_VOL_CTRL = 0x20,
  UDA_PGA_CTRL,
  UDA_ADC_CTRL,
  UDA_AGC_CTRL,
  UDA_TOTAL_REG
}UDA1380_REG;

/* System Register Data Set */
unsigned short UDA_sys_regs_dat[]={
  /* UDA_EVALM_CLK */
  0xF <<8 | 0x3<<4 | 1,
  /* UDA_BUS_CTRL */
  0x00,
  /* UDA_POWER_CTRL */
  1<<15 | 1 <<13 | 1<<10 | 1<<8,
  /* UDA_ANALOG_CTRL */
  0x0000,
  /* UDA_HPAMP_CTRL */
  1<<9 | 2
};

/* System Register Data Set */
unsigned short UDA_interfil_regs_dat[]={
  /* UDA_MASTER_VOL_CTRL */
  0x0000, /* MAX volume */
  /* UDA_MIXER_VOL_CTRL */
  0x0000,
  /* UDA_MODE_CTRL */
  0x0000,
  /* UDA_MUTE_CTRL */
  2<<8 | 2,
  /* UDA_MIXER_FILTER_CTRL */
  0x0000,
};
/* decimator Register Data Set */
unsigned short UDA_decimator_regs_dat[]={
  /* UDA_DEC_VOL_CTRL */
  0x0000,
  /* UDA_PGA_CTRL */
  0x0000,
  /* UDA_ADC_CTRL */
  0x0000,
  /* UDA_AGC_CTRL */
  0x0000

};
void delay(UNS_32 cnt);
void delay(UNS_32 cnt)
{
    UNS_32 i = cnt;
    while (i != 0) i--;
    return;
}
/* UDA register read function */
unsigned short UDA_Reg_read(UDA1380_REG reg);
unsigned short UDA_Reg_read(UDA1380_REG reg){
  I2C_M_SETUP_Type transferMCfg;
  unsigned char TXdata, RXdata[2];
  unsigned short result=0;
	TXdata = reg;
//    transferMCfg.addr_mode = ADDR7BIT;
    transferMCfg.sl_addr7bit = (0x34>>1);//I2CDEV_UDA1380_ADDR
    transferMCfg.tx_data = &TXdata;
    transferMCfg.tx_length = 1;
    transferMCfg.rx_data = &RXdata[0];
    transferMCfg.rx_length = 2;
    transferMCfg.retransmissions_max = 5;
    I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);
    //while (( i2c_mtxrx_setup.status & I2C_SETUP_STATUS_DONE) == 0);
    result = RXdata[0]<<8 | RXdata[1];
	return result;
}

/* UDA register write function */
void UDA_Reg_write(UDA1380_REG reg, unsigned short value);
void UDA_Reg_write(UDA1380_REG reg, unsigned short value){
  I2C_M_SETUP_Type transferMCfg;
  UNS_8  tx_data[3];
  tx_data[0] = reg;
  tx_data[1] = value>>8;
  tx_data[2] = value&0xFF;
//  i2c_mtx_setup.addr_mode = ADDR7BIT;
  transferMCfg.sl_addr7bit = (0x34>>1);//I2CDEV_UDA1380_ADDR
  transferMCfg.tx_data = &tx_data[0];
  transferMCfg.tx_length = 3;
  transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
  transferMCfg.retransmissions_max = 5;
  I2C_MasterTransferData(LPC_I2C, &transferMCfg, I2C_TRANSFER_POLLING);
  //while (( i2c_mtx_setup.status & I2C_SETUP_STATUS_DONE) == 0);
  delay(10000);
}
BOOL_32 UDA1380_init();
BOOL_32 UDA1380_init(){
  unsigned short temp;
  int i;

  LPC_CGU->IDIVC_CTRL = (0x8<<24) | (7<<2) |(1<<11);
  LPC_CGU->BASE_OUT_CLK = 0x0D<<24 | (1<<11);
  LPC_SCU->SFSCLK_3 = 1 | (0x3<<3);

  /* Reset device */
  UDA_Reg_write(0x7F, 0x00);

    /* Sys regs init */
	UDA_Reg_write(UDA_EVALM_CLK , 0x0502);
  for(i=1; i<5; i++){
    UDA_Reg_write(UDA_EVALM_CLK + i, UDA_sys_regs_dat[i]);
    temp = UDA_Reg_read(UDA_EVALM_CLK + i);
    if(temp!=UDA_sys_regs_dat[i]) return FALSE;
  }

  /* interfilter regs init */
  for(i=0; i<5; i++){
    UDA_Reg_write(UDA_MASTER_VOL_CTRL + i, UDA_interfil_regs_dat[i]);
    temp = UDA_Reg_read(UDA_MASTER_VOL_CTRL + i);
    if(temp!=UDA_interfil_regs_dat[i]) return FALSE;
  }
  /* decimator regs init */
  for(i=0; i<4; i++){
    UDA_Reg_write(UDA_DEC_VOL_CTRL + i, UDA_decimator_regs_dat[i]);
    temp = UDA_Reg_read(UDA_DEC_VOL_CTRL + i);
    if(temp!=UDA_decimator_regs_dat[i]) return FALSE;
  }
	UDA_Reg_write(UDA_EVALM_CLK, UDA_sys_regs_dat[0]);

  return TRUE;
}
/**
 * Audio API
 */
LPC_I2Sn_Type *I2S = LPC_I2S0;
#define I2SIRQ		I2S0_IRQn
#define AudioIrq	I2S0_IRQHandler
#define I2S_TX_LEVEL	4
/** Audio max packet count. */
#define AUDIO_MAX_PC	10

/** Audio Data Buffer, used by BSP output and ISO transfer.
	This is a store, not all space will be used in application. */
uint8_t audio_buffer[2048] __attribute__ ((aligned(4)));
uint32_t audio_buffer_size = 0;
uint32_t audio_buffer_rd_index = 0;
uint32_t audio_buffer_wr_index = 0;
uint32_t audio_buffer_count = 0;

void Audio_Init(uint32_t samplefreq)
{
	I2S_MODEConf_Type I2S_ClkConfig;
	I2S_CFG_Type I2S_ConfigStruct;

	Audio_Reset_Data_Buffer();
	NVIC_DisableIRQ(I2SIRQ);

	switch(samplefreq){
	case 11025:
	case 22050:
	case 44100:
		audio_buffer_size = 1764;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	default:
		audio_buffer_size = samplefreq * 4 * AUDIO_MAX_PC / 1000;
		break;
	}
	
	/* Reset UDA1380 on board Hitex */
#if (BOARD == BOARD_HITEX_A4)
	scu_pinmux(8,2,MD_PUP, FUNC0);
	GPIO_SetDir(4, 1<<2, 1);
	GPIO_ClearValue(4, 1<<2);
#else
	scu_pinmux(0xE,9,MD_PUP, FUNC4);
	GPIO_SetDir(7, 1<<9, 1);
	GPIO_ClearValue(7, 1<<9);
#endif

/* Initialize I2S peripheral ------------------------------------*/
	/* Init I2C */
	I2C_Init(LPC_I2C, 100000);
	/* Enable Slave I2C operation */
	I2C_Cmd(LPC_I2C, ENABLE);
	/* Init UDA1380 CODEC */
	UDA1380_init();
/* Initialize I2S peripheral ------------------------------------*/
	scu_pinmux(3,0, MD_PUP, 2);
	scu_pinmux(6,0, MD_PUP, 0);

	scu_pinmux(7,2, MD_PUP, 2);
	scu_pinmux(6,2, MD_PUP, 3);

	scu_pinmux(7,1, MD_PUP, 2);
	scu_pinmux(6,1, MD_PUP, 3);

	I2S_Init(I2S);

	/* setup:
	 * 		- wordwidth: 16 bits
	 * 		- stereo mode
	 * 		- master mode for I2S_TX
	 * 		- Frequency = 44.1 kHz
	 */

	/* Audio Config*/
	I2S_ConfigStruct.wordwidth = I2S_WORDWIDTH_16;
	I2S_ConfigStruct.mono = I2S_STEREO;
	I2S_ConfigStruct.stop = I2S_STOP_ENABLE;
	I2S_ConfigStruct.reset = I2S_RESET_ENABLE;
	I2S_ConfigStruct.ws_sel = I2S_MASTER_MODE;
	I2S_ConfigStruct.mute = I2S_MUTE_DISABLE;
	I2S_Config(I2S,I2S_TX_MODE,&I2S_ConfigStruct);

	/* Clock Mode Config*/
	I2S_ClkConfig.clksel = I2S_CLKSEL_FRDCLK;
	I2S_ClkConfig.fpin = I2S_4PIN_DISABLE;
	I2S_ClkConfig.mcena = I2S_MCLK_DISABLE;
	I2S_ModeConfig(I2S,&I2S_ClkConfig,I2S_TX_MODE);

	I2S_FreqConfig(I2S, samplefreq, I2S_TX_MODE);

	I2S_Stop(I2S, I2S_TX_MODE);

	/* TX FIFO depth is 4 */
	I2S_IRQConfig(I2S,I2S_TX_MODE,I2S_TX_LEVEL);
	I2S_IRQCmd(I2S,I2S_TX_MODE,ENABLE);
	I2S_Start(I2S);
	NVIC_EnableIRQ(I2SIRQ);
}

void AudioIrq(void);
void AudioIrq(void)
{
	uint32_t txlevel,i;
	txlevel = I2S_GetLevel(I2S,I2S_TX_MODE);
	if(txlevel <= I2S_TX_LEVEL)
	{
		for(i=0;i<8-txlevel;i++)
		{
			if(audio_buffer_count >= 4) /*has enough data */
			{
				audio_buffer_count -= 4;
				I2S->TXFIFO = *(uint32_t *)(audio_buffer + audio_buffer_rd_index);
				audio_buffer_rd_index+=4;
				if(audio_buffer_rd_index >= audio_buffer_size) 
					audio_buffer_rd_index -= audio_buffer_size;
			}else{
				I2S->TXFIFO = 0;
			}
			
			/*Skip some samples if buffer run writting too fast. */
			if((audio_buffer_size != 0) && (audio_buffer_count >= (audio_buffer_size/2)))
			{
				audio_buffer_count-=4;
				audio_buffer_rd_index+=4;
				if(audio_buffer_rd_index >= audio_buffer_size)
					audio_buffer_rd_index -= audio_buffer_size;
			}
		}
	}
}

/** Provide Audio buffer for next iso transfer */
uint32_t Audio_Get_ISO_Buffer_Address(uint32_t last_packet_size)
{
	audio_buffer_wr_index += last_packet_size;
	audio_buffer_count += last_packet_size;

	if(audio_buffer_count > audio_buffer_size)
	{
		Audio_Reset_Data_Buffer();
	}
	
	if(audio_buffer_wr_index >= audio_buffer_size)
		audio_buffer_wr_index -= audio_buffer_size;
		
	return (uint32_t)&audio_buffer[audio_buffer_wr_index];
}
/* Reset Audio Data Buffer. */
void Audio_Reset_Data_Buffer(void)
{
	audio_buffer_count = 0;
	audio_buffer_wr_index = 0;
	audio_buffer_rd_index = 0;
}

void USB_Connect(void) {};
void Joystick_Init(void) {}
uint8_t Joystick_GetStatus(void)
{
	return NO_BUTTON_PRESSED;
}

extern uint8_t DiskImage[];
#include "../DataRam.h"
#include <string.h>
/** Initialize the Mass Storage, depend on implementation of board, it can be
 * SPIFI, NOR Flash, NAND Flash, RAM.. */
void MassStorage_Init(void)
{
}

uint32_t MassStorage_Cache_Flush(bool appcall);
/** Program Mass Memory */
void MassStorage_Write(uint32_t diskpos, void* buffer, uint32_t size)
{
	uint32_t block = (uint32_t)diskpos/size;
	if (block < DATA_RAM_PHYSICAL_SIZE/size)
	{
		memcpy(&DiskImage[block*size], buffer, size);
	}
}

void MassStorage_Read(uint32_t diskpos, void* buffer, uint32_t size){
	uint32_t block = (uint32_t)diskpos/size;
	if (block < DATA_RAM_PHYSICAL_SIZE/size)
	{
		memcpy(buffer, &DiskImage[block*size], size);
	}else
		memset(buffer, 0, size);
}

/** Verify Mass Memory with other Memory */
bool MassStorage_Verify(void*dest, void* source, uint32_t size){
	return true;

}
/** Get Mass Storage Size */
uint32_t MassStorage_GetCapacity(void)
{
	return DATA_RAM_VIRTUAL_SIZE;	//use 2 first sectors
}

uint32_t MassStorage_Cache_Flush(bool appcall)
{
	return 0;
}

bool MassStorage_FlushTimeOut(void)
{
	return false;
}
#endif
