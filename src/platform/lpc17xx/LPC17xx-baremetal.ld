/* Start the user code at the top of flash - not compatible with the USB
 * bootloader.
 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
  RAM (rwx) : ORIGIN = 0x100000C8, LENGTH = 0x7F38
}

GROUP(-lstdc++ -lsupc++ -lm -lc -lnosys -lgcc)
INCLUDE "platform/lpc17xx/LPC17xx-base.ld"
