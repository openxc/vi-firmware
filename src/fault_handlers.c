#include <stdio.h>
#include "LPC17xx.h"
#include "core_cm3.h"

// From Joseph Yiu, minor edits by FVH
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file xxx.s
void hard_fault_handler_c (unsigned int * hardfault_args) {
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;

  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);

  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);

  printf ("\n\n[Hard fault handler - all numbers in hex]\n");
  printf ("R0 = %x\n", stacked_r0);
  printf ("R1 = %x\n", stacked_r1);
  printf ("R2 = %x\n", stacked_r2);
  printf ("R3 = %x\n", stacked_r3);
  printf ("R12 = %x\n", stacked_r12);
  printf ("LR [R14] = %x  subroutine call return address\n", stacked_lr);
  printf ("PC [R15] = %x  program counter\n", stacked_pc);
  printf ("PSR = %x\n", stacked_psr);
  printf ("BFAR = %x\n", (*((volatile unsigned int *)(0xE000ED38))));
  printf ("CFSR = %x\n", (*((volatile unsigned int *)(0xE000ED28))));
  printf ("HFSR = %x\n", (*((volatile unsigned int *)(0xE000ED2C))));
  printf ("DFSR = %x\n", (*((volatile unsigned int *)(0xE000ED30))));
  printf ("AFSR = %x\n", (*((volatile unsigned int *)(0xE000ED3C))));
  /* printf ("SCB_SHCSR = %x\n", SCB->SHCSR); */

  while (1);
}
