/* Start the user code 64KB into flash, as the USB bootloader expects. */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x10000, LENGTH = 512K - 0x10000
  RAM (rwx) : ORIGIN = 0x100000C8, LENGTH = 0x7F38
}

GROUP(-lstdc++ -lsupc++ -lm -lc -lnosys -lgcc)
INCLUDE "platform/lpc17xx/LPC17xx-base.ld"
