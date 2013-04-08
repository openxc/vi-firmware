/* Start the user code 64KB into flash, as the USB bootloader expects. */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x10000, LENGTH = 512K - 0x10000
  RAM (rwx) : ORIGIN = 0x100000C8, LENGTH = 0x7F38

  USB_RAM(rwx) : ORIGIN = 0x2007C000, LENGTH = 16K
  ETH_RAM(rwx) : ORIGIN = 0x20080000, LENGTH = 16K
}

GROUP(-lstdc++ -lsupc++ -lm -lc -lnosys -lgcc)
INCLUDE "LPC17xx-base.ld"
