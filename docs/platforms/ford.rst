Ford Reference Vehicle interface
================================

The Ford Reference VI is an open source hardware implementation of a VI, and the
complete documentation is available at `vi.openxcplatform.com
<http://vi.openxcplatform.com>`_.

To build for the Ford reference VI, compile with the flag
``PLATFORM=FORDBOARD``.

Flashing a Pre-compiled Firmware
--------------------------------

Pre-compiled binaries (built with the ``BOOTLOADER`` flag enabled, see all
:doc:`compiler flags </compile/makefile-opts>`) are compatible with the `OpenLPC USB bootloader
<https://github.com/openxc/openlpc-USB_Bootloader>`_  - follow the instructions
for `Flashing User Code
<https://github.com/openxc/openlpc-USB_Bootloader#flashing-user-code>`_ to
update the vehicle interface.

Compiling
---------

USB Bootloader
""""""""""""""

If you are running a supported bootloader, you don't need any special
programming hardware. Compile the firmware to run under the bootloader:

.. code-block:: sh

   $ fab reference build

The compiled firmware will be located at
``build/lpc17xx/vi-firmware-lpc17xx.bin``. See `reference VI programming
instructions <http://vi.openxcplatform.com/firmware/programming/usb.html>`_ to
find out how to re-flash the VI.

Bare Metal
""""""""""

Attach a JTAG adapter to your computer and the VI, then compile and flash:

.. code-block:: sh

    $ export PLATFORM=FORDBOARD
    $ export BOOTLOADER=0
    $ make clean
    $ make -j4
    $ make flash

The config files in this repository assume your JTAG adapter is the
Olimex ARM-USB-OCD unit. If you have a different unit, modify the
``src/lpc17xx/lpc17xx.mk`` Makefile to load your programmer's OpenOCD
configuration.

UART
----

The software configuration is identical to the :doc:`Blueboard <blueboard>`. The
reference VI includes an RN-41 on the PCB attached to the RX, TX, CTS and RTS
pins, in addition to the UART status pin.

When a Bluetooth host pairs with the RN-42 and opens an RFCOMM connection, pin
0.18 will be pulled high and the VI will being streaming vehicle data over UART.

Debug Logging
-------------

In most cases the logging provided via USB is sufficient, but if you are doing
low-level development and need the simpler UART interface, you can enable it
with the ``DEFAULT_LOGGING_OUTPUT="UART"`` build option.

Logging will be on UART0, which is exposed on the bottom of the board at J3, a
5-pin ISP connector.

LED Lights
----------

The reference VI has 2 RGB LEDs. If the LEDs are a dim green and red, then the
firmware was not flashed properly and the board is not running.

**LED A**

- CAN activity detected - Blue
- No CAN activity on either bus - Orange

**LED B**

- USB connected, Bluetooth not connected - Green
- Bluetooth connected, USB in either state - Blue
- Neither USB or Bluetooth connected - Off

Bootloader
----------

The `OpenLPC USB bootloader <https://github.com/openxc/openlpc-USB_Bootloader>`_
is tested and working, and enables the LPC17xx to appear as a USB drive. See the
documentation in that repository for instructions on how to flash the bootloader
(a JTAG programmer is required). The reference VI from Ford is pre-programmed
with this bootloader.
