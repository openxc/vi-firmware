Digilent chipKIT Max32
=======================

To build for the FleetCarma data logger, compile with the flag
``PLATFORM=FLEETCARMA``.

Flashing a Pre-compiled Firmware
--------------------------------

Assuming your board has already been flashed with the OpenXC fork of the
`PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_,
you must attach an FTDI between your computer and the device.

FTDI Cable
^^^^^^^^^^

TODO document pins for UART flashing

Uploading Script
^^^^^^^^^^^^^^^^

Once you have the FTDI cable attached to your computer and to the FleetCarma
board, follow the same steps to upload as for the :doc:`chipKIT Max32 <max32>`.

Bootloader
----------

The FleetCarma uses the "default" pins for the CAN controller. The
`PIC32 avrdude bootloader <https://github.com/openxc/PIC32-avrdude-bootloader>`_
works well, but we needed to add a custom board definition to switch from the
"alternative" pins (used on the chipKIT Max32) to the default pins.

The fork of the PIC32-avrdude-bootloader repository under the openxc account has
the proper board definition. You will need to install MPLAB X, open the
bootloader project, compile and flash the FleetCarma board via ICSP. We've
tested this from Windows and Linux using the Microchip PICkit 3 programmer.

Compiling
---------

The instructions for compiling from source are identical to the :doc:`chipKIT
Max32 <max32>` except that ``PLATFORM=FLEETCARMA`` instead of ``CHIPKIT``. If
you will not be using the avrdude bootloader and will be flashing directly via
ICSP, make sure to also compile with ``BOOTLOADER=0`` to enable the program to
run on bare metal.

USB
---

The micro-USB port on the board is used to send and receive OpenXC messages.

UART
----

On the FleetCarma device, ``UART1A`` is used for OpenXC output at the 230000
baud rate. Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low
by the receiving device before data will be sent.

TODO add pinout of expansion header, probably a picture

UART data is sent only if pin 0.58 (or PORTB BIT 4, RB4) is pulled high (to
5vv). If you are using a Bluetooth module like the `BlueSMiRF
<https://www.sparkfun.com/products/10269>`_ from SparkFun, you need to hard-wire
5v into this pin to actually enabling UART. To disable UART, pull this pin low
or leave it floating.

Debug Logging
-------------

On the FleetCarma board, logging is on UART3A at 115200 baud (if the firmware
was compiled with ``DEBUG=1``).

LED Lights
-----------

The FleetCarma board has 2 user controllable LEDs. When CAN activity is
detected, the green LED will be enabled. When USB or Bluetooth is connected, the
blue LED will be enabled.
