===========
Bootloaders
===========

For those who don't have special microcontroller programming hardware, we strive
to make the vehicle interface firmware compatible with USB bootloaders. This
allows reflashing the firmware by copying a file over to a simulated USB drive,
or by using the popular avrdude tool.

Digilent chipKIT Max32
======================

The `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_ is tested and working and
allows flashing over USB with ``avrdude``. All stock chipKITs are programmed
with a compatible bootloader at the factory.

NXP LPC17xx
===========

The `OpenLPC USB bootloader <https://github.com/openxc/openlpc-USB_Bootloader>`_
is tested and working, and enables the LPC17xx to appear as a USB drive. See the
documentation in that repository for instructions on how to flash the bootloader
(a JTAG programmer is required).

Once you have a compiled firmware ``.bin`` file (either from an OEM or compiled
from source), hold down the bootloader button on the board and attach the USB
connector to your computer. When the LPC17xx drive appears, delete the existing
firmware file and copy the new one. Eject the device and power cycle it - you
should not be running the freshly compiled firmware.
