Ford Prototype Vehicle Interface
================================

To build for the Ford prototype, compile with the flag ``PLATFORM=FORDBOARD``.

Flashing a Pre-compiled Firmware
--------------------------------

Pre-compiled binaries (built with the ``BOOTLOADER`` flag enabled, see all
`compiler flags <compiling>`_) are compatible with the `OpenLPC USB bootloader
<https://github.com/openxc/openlpc-USB_Bootloader>`_  - follow the instructions
for `Flashing User Code
<https://github.com/openxc/openlpc-USB_Bootloader#flashing-user-code>`_ to
update the vehicle interface.

Bootloader
----------

The `OpenLPC USB bootloader <https://github.com/openxc/openlpc-USB_Bootloader>`_
is tested and working, and enables the LPC17xx to appear as a USB drive. See the
documentation in that repository for instructions on how to flash the bootloader
(a JTAG programmer is required).

Compiling
---------

USB Bootloader
""""""""""""""

If you are running a :doc:`supported bootloader </installation/bootloaders>`,
you don't need any special programming hardware. Compile the firmware to run
under the bootloader:

.. code-block:: sh

   $ make clean
   $ PLATFORM=FORDBOARD BOOTLOADER=1 make -j4

The compiled firmware will be located at
``build/lpc17xx/cantranslator-lpc17xx.bin``. See the :doc:`bootloaders
</installation/bootloaders>` page for instructions on how to load the firmware.

Bare Metal
""""""""""

Once the :doc:`dependencies </installation/installation>` are installed, attach a
JTAG adapter to your computer and the CAN translator, then compile and flash:

.. code-block:: sh

    $ make clean
    $ PLATFORM=FORDBOARD make -j4
    $ PLATFORM=FORDBOARD make flash

The config files in this repository assume your JTAG adapter is the
Olimex ARM-USB-OCD unit. If you have a different unit, modify the
``src/lpc17xx/lpc17xx.mk`` Makefile to load your programmer's OpenOCD
configuration.

UART
----

The software configuration is identical to the :doc:`Blueboard <blueboard>`. The
Ford prototype includes an RN-41 on the PCB attached to the RX, TX, CTS and RTS
pins, in addition to the UART status pin.

When a Bluetooth host pairs with the RN-42 and opens an RFCOMM connection, pin
0.18 will be pulled high and the VI will being streaming vehicle data over UART.

Debug Logging
-------------

Logging will be on UART0, which is exposed on the bottom of the board at J3, a
5-pin ISP connector.

LED Lights
----------

The Ford prototype has 2 RGB LEDs.

**LED A**

- CAN activity detected - Blue
- No CAN activity on either bus - Off

**LED B**

- USB connected, Bluetooth not connected - Green
- Bluetooth connected, USB in either state - Blue
- Neither USB or Bluetooth connected - Off
