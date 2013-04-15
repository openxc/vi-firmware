/*
 * Derived from startup code originally published by Jakub Piotr Cłapa,
 * Copyright (c) 2010 LoEE under the new BSD license.
 *
 * See also: http://bitbucket.org/jpc/lpc1768/
 *
 * NVIC handler names from NXP UM10360, comments taken from Opendous Inc. under
 * the following license:
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that the
 * copyright notice and this permission notice and warranty disclaimer appear in
 * supporting documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.

 * The author disclaim all warranties with regard to this software, including
 * all implied warranties of merchantability and fitness.  In no event shall the
 * author be liable for any special, indirect or consequential damages or any
 * damages whatsoever resulting from loss of use, data or profits, whether in an
 * action of contract, negligence or other tortious action, arising out of or in
 * connection with the use or performance of this software.
 */
#include "LPC17xx.h"

// Defined in linker script
extern void __stack(void);
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

// Defined by user application
extern int main (void);

// Dummy handler.
void Default_Handler (void) { while (1); }

// Weakly bind all interrupt vectors to the dummy handler.
void NMI_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void MemManage_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void BusFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UsageFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SVC_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void DebugMon_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PendSV_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SysTick_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void WDT_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UART0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UART1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UART2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UART3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PWM1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2C2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SPI_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SSP0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SSP1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PLL0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void RTC_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void EINT0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void EINT1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void EINT2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void EINT3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ADC_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void BOD_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USB_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void CAN_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void DMA_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2S_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ENET_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void RIT_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void MCPWM_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void QEI_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PLL1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USBActivity_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void CANActivity_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));

// The signature of Cortex-M3 interrupt handlers.
typedef void (* const Interrupt_Handler_P)(void);

// Forward declaration of Reset_Handler so we can refernece interrupt_vectors
// when setting the VTOR in the handler.
void Reset_Handler(void);

// Interrupt vectors table
__attribute__ ((section(".isr_vector")))
Interrupt_Handler_P interrupt_vectors[] = {
  &__stack,                     // the first  word contains  the initial
                                // stack pointer  the hardware  loads it
                                // to the  SP register before  the first
                                // instruction
  // Standard Cortex-M3 interrupts:
  Reset_Handler,
  NMI_Handler,
  HardFault_Handler,
  MemManage_Handler,
  BusFault_Handler,
  UsageFault_Handler,
  0,
  0,
  0,
  0,
  SVC_Handler,
  DebugMon_Handler,
  0,
  PendSV_Handler,
  SysTick_Handler,
  // LPC17xx Interrupts: Interrupt ID - Exception Number - Vector Offset - Description and Flags

  WDT_IRQHandler,       // 0  16 0x40   WDT Watchdog Interrupt (WDINT)
  TIMER0_IRQHandler,    // 1  17 0x44   Timer 0 Match 0 - 1 (MR0, MR1)
        //      Capture 0 - 1 (CR0, CR1)
  TIMER1_IRQHandler,    // 2  18 0x48   Timer 1 Match 0 - 2
        //      (MR0, MR1, MR2), Capture 0 - 1 (CR0, CR1)
  TIMER2_IRQHandler,    // 3  19 0x4C   Timer 2 Match 0-3
        //      Capture 0-1
  TIMER3_IRQHandler,    // 4  20 0x50   Timer 3 Match 0-3
        //      Capture 0-1
  UART0_IRQHandler, // 5  21 0x54   UART0 Rx Line Status (RLS)
        //      Transmit Holding Register Empty (THRE)
        //      Rx Data Available (RDA)
        //      Character Time-out Indicator (CTI)
        //      End of Auto-Baud (ABEO)
        //      Auto-Baud Time-Out (ABTO)
  UART1_IRQHandler, // 6  22 0x58   UART1 Rx Line Status (RLS)
        //      Transmit Holding Register Empty (THRE)
        //      Rx Data Available (RDA)
        //      Character Time-out Indicator (CTI)
        //      Modem Control Change
        //      End of Auto-Baud (ABEO)
        //      Auto-Baud Time-Out (ABTO)
  UART2_IRQHandler, // 7  23 0x5C   UART 2 Rx Line Status (RLS)
        //      Transmit Holding Register Empty (THRE)
        //      Rx Data Available (RDA)
        //      Character Time-out Indicator (CTI)
        //      End of Auto-Baud (ABEO)
        //      Auto-Baud Time-Out (ABTO)
  UART3_IRQHandler, // 8  24 0x60   UART 3 Rx Line Status (RLS)
        //      Transmit Holding Register Empty (THRE)
        //      Rx Data Available (RDA)
        //      Character Time-out Indicator (CTI)
        //      End of Auto-Baud (ABEO)
        //      Auto-Baud Time-Out (ABTO)
  PWM1_IRQHandler,  // 9  25 0x64   PWM1 Match 0 - 6 of PWM1
        //      Capture 0-1 of PWM1
  I2C0_IRQHandler,  // 10 26 0x68   I2C0 SI (state change)
  I2C1_IRQHandler,  // 11 27 0x6C   I2C1 SI (state change)
  I2C2_IRQHandler,  // 12 28 0x70   I2C2 SI (state change)
  SPI_IRQHandler,       // 13 29 0x74   SPI SPI Interrupt Flag (SPIF)
        //      Mode Fault (MODF)
  SSP0_IRQHandler,  // 14 30 0x78   SSP0 Tx FIFO half empty of SSP0
        //      Rx FIFO half full of SSP0
        //      Rx Timeout of SSP0
        //      Rx Overrun of SSP0
  SSP1_IRQHandler,  // 15 31 0x7C   SSP 1 Tx FIFO half empty
        //      Rx FIFO half full
        //      Rx Timeout
        //      Rx Overrun
  PLL0_IRQHandler,  // 16 32 0x80   PLL0 (Main PLL) PLL0 Lock (PLOCK0)
  RTC_IRQHandler,       // 17 33 0x84   RTC Counter Increment (RTCCIF)
        //      Alarm (RTCALF)
  EINT0_IRQHandler, // 18 34 0x88   External Interrupt External Interrupt 0 (EINT0)
  EINT1_IRQHandler, // 19 35 0x8C   External Interrupt External Interrupt 1 (EINT1)
  EINT2_IRQHandler, // 20 36 0x90   External Interrupt External Interrupt 2 (EINT2)
  EINT3_IRQHandler, // 21 37 0x94   External Interrupt External Interrupt 3 (EINT3).
        //      Note: EINT3 channel is shared with GPIO interrupts
  ADC_IRQHandler,       // 22 38 0x98   ADC A/D Converter end of conversion
  BOD_IRQHandler,       // 23 39 0x9C   BOD Brown Out detect
  USB_IRQHandler,       // 24 40 0xA0   USB USB_INT_REQ_LP, USB_INT_REQ_HP, USB_INT_REQ_DMA
  CAN_IRQHandler,       // 25 41 0xA4   CAN CAN Common, CAN 0 Tx, CAN 0 Rx, CAN 1 Tx, CAN 1 Rx
  DMA_IRQHandler,       // 26 42 0xA8   GPDMA IntStatus of DMA channel 0, IntStatus of DMA channel 1
  I2S_IRQHandler,       // 27 43 0xAC   I2S irq, dmareq1, dmareq2
  ENET_IRQHandler,  // 28 44 0xB0   Ethernet WakeupInt, SoftInt, TxDoneInt, TxFinishedInt, TxErrorInt,
        //      TxUnderrunInt, RxDoneInt, RxFinishedInt, RxErrorInt, RxOverrunInt.
  RIT_IRQHandler,       // 29 45 0xB4   Repetitive Interrupt Timer (RITINT)
  MCPWM_IRQHandler, // 30 46 0xB8   Motor Control PWM IPER[2:0], IPW[2:0], ICAP[2:0], FES
  QEI_IRQHandler,       // 31 47 0xBC   Quadrature Encoder INX_Int, TIM_Int, VELC_Int, DIR_Int, ERR_Int, ENCLK_Int,
        //      POS0_Int, POS1_Int, POS2_Int, REV_Int, POS0REV_Int, POS1REV_Int, POS2REV_Int
  PLL1_IRQHandler,  // 32 48 0xC0   PLL1 (USB PLL) PLL1 Lock (PLOCK1)
  USBActivity_IRQHandler,    // 33 49 0xC4   USB Activity Interrupt USB_NEED_CLK
  CANActivity_IRQHandler,    // 34 50 0xC8   CAN Activity Interrupt CAN1WAKE, CAN2WAKE
};

void Reset_Handler(void) {
  SystemInit();
  // SystemInit clears the Vector Table Offset Register (VTOR) back to 0. If
  // we're running bare metal, the VTOR should be 0. If we're running behind the
  // USB bootloader, however, the interrupt vectors are not at to top of flash
  // anymore. We set the VTOR in the bootloader, but since we need to run
  // SystemInit here to power up the peripherals and clock, we have to re-set it
  // to the address of interrupt_vectors.
  SCB->VTOR = (uint32_t) interrupt_vectors;

  uint32_t *s, *d;
  // Copy initialization data to RAM (.data section)
  s = &__etext;
  d = &__data_start__;
  while (d < &__data_end__) *d++ = *s++;
  // Zero the remaining allocated RAM (.bss section)
  d = &__bss_start__;
  while (d < &__bss_end__)  *d++ = 0;

  // Everything is ready. Run the user program.
  main();
  while(1); // in case main() fails
}
