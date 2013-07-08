CrossChasm C5 Interface
=======================

CrossChasm's C5 OBD interface is compatible with the OpenXC cantranslator
firmware. To build for the C5, compile with the flag ``PLATFORM=CROSSCHASM_C5``.

CrossChasm has made the C5 `available for purchase
<http://crosschasm.com/SolutionCenter/OpenXC.aspx>`_ from their website, and it
comes pre-loaded with the correct bootloader, so you don't need any additional
hardware to load the OpenXC firmware.

The C5 connects to the `CAN1 bus pins
<http://openxcplatform.com/vehicle-interface/#obd-pins>`_ on the OBD-II
connector.

Flashing a Pre-compiled Firmware
--------------------------------

Assuming your C5 has the :ref:`bootloader <bootloader>` already flashed, once
you have the USB cable attached to your computer and to the C5, follow the same
steps to upload as for the :doc:`chipKIT Max32 <max32>`.

The C5 units offered directly from the `CrossChasm website
<http://crosschasm.com/SolutionCenter/OpenXC.aspx>`_ are pre-programmed with the
bootloader.

.. _bootloader:

Bootloader
----------

The C5 can be flashed with the same `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_, as the chipKIT.

The OpenXC fork (the previous link) defines a `CROSSCHASM_C5` configuration that
works as a USB bootloader for this unit. Once the bootloader is flashed, there
is a 5 second window when the unit powers on when it listens for avrdude
commands via a CDC modem interface exposed over USB (e.g. in Linux it may show
up as something like `/dev/ACM0`, and you can treat this just as if it were a
serial device).

The C5 units offered directly from the `CrossChasm website
<http://crosschasm.com/SolutionCenter/OpenXC.aspx>`_ are pre-programmed with the
bootloader.

If you need to reflash the bootloader yourself, a ready-to-go .hex file is
available in the `GitHub repository
<https://raw.github.com/openxc/PIC32-avrdude-bootloader/master/bootloaders/CrossChasm-C5-USB.hex>`_
and you can flash it with MPLAB IDE/IPE and an ICSP programmer like the
Microchip PICkit 3. You can also build it from source in MPLAB by using the
`CrossChasm C5` configuration.

Compiling
---------

The instructions for compiling from source are identical to the :doc:`chipKIT
Max32 <max32>` except that ``PLATFORM=CROSSCHASM_C5`` instead of ``CHIPKIT``.

If you will not be using the avrdude bootloader and will be flashing directly
via ICSP, make sure to also compile with ``BOOTLOADER=0`` to enable the program
to run on bare metal.

USB
---

The micro-USB port on the board is used to send and receive OpenXC messages.

UART
----

On the C5, ``UART1A`` is used for OpenXC output at the 230000 baud rate.
Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low by the
receiving device before data will be sent.

TODO add pinout of expansion header, probably a picture

UART data is sent only if pin 0.58 (or PORTB BIT 4, RB4) is pulled high (to
5vv). If you are using a Bluetooth module like the `BlueSMiRF
<https://www.sparkfun.com/products/10269>`_ from SparkFun, you need to hard-wire
5v into this pin to actually enabling UART. To disable UART, pull this pin low
or leave it floating.

Debug Logging
-------------

On the C5, logging is on UART3A at 115200 baud (if the firmware was compiled
with ``DEBUG=1``).

LED Lights
-----------

The C5 has 2 user controllable LEDs. When CAN activity is detected, the green
LED will be enabled. When USB or Bluetooth is connected, the blue LED will be
enabled.
