CrossChasm C5 Bluetooth LE Interface
=======================

CrossChasm's BTLE_C5 OBD interface is compatible with the OpenXC VI
firmware. To build for the C5, compile with the flag ``PLATFORM=CROSSCHASM_BTLE_C5``.

The BTLE_C5 connects to the CAN1 bus pins
'<http://openxcplatform.com/vehicle-interface/#obd-pins>' on the OBD-II
connector.

Bluetooth Low Energy Specifications
--------------------------------
The Bluetooth Low Energy or BLE protocol supports communication through a GATT server - client
architecture. The device hosts a single service with following read and write characteristic UUIDs -

* Service-        6800-d38b-423d-4bdb-ba05-c9276d8453e1

* Write Characteristic  6800-d38b-5262-11e5-885d-feff819cdce2

* Notify Characteristic 6800-d38b-5262-11e5-885d-feff819cdce3

The phone application or the GATT client should enable 
notifications on the notify characteristic in order to recieve 
OpenXC messages from the device. The write characteristic
can be used to send commands and configuration messages to the device.
A detailed description of the message format can be found at
<https://github.com/openxc/openxc-message-format> and

Flashing a Pre-compiled Firmware
--------------------------------

Assuming your BTLE_C5 has the :ref:`bootloader <bootloader>` already flashed, once
you have the USB cable attached to your computer and to the BTLE_C5, follow the same
steps to upload as for the :doc:`chipKIT Max32 <max32>`.

Bootloader
----------

The BTLE_C5 can be flashed with the same `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_, as the chipKIT.

The OpenXC fork of the bootloader (the previous link) defines a `CROSSCHASM_C5` configuration that
exposes a CDC/ACM serial port function over USB. Once the bootloader is flashed, there
is a 5 second window when the unit powers on when it will accept bootloader
commands.

In Linux and OS X it will show up as something like `/dev/ACM0`, and you can treat this
just as if it were a serial device.

In Windows, you will need to install the `stk500v2.inf
<https://raw.github.com/openxc/PIC32-avrdude-bootloader/master/Stk500v2.inf>`
driver before the CDC/ACM modem will show up - download that file, right click
and choose Install. The C5 should now show up as a COM port for for 5 seconds on
bootup.

If you need to reflash the bootloader yourself, a ready-to-go .hex file is
available in the `GitHub repository
<https://raw.github.com/openxc/PIC32-avrdude-bootloader/master/bootloaders/CrossChasm-C5-USB.hex>`_
and you can flash it with MPLAB IDE/IPE and an ICSP programmer like the
Microchip PICkit 3. You can also build it from source in MPLAB by using the
`CrossChasm C5` configuration.

Compiling
---------

The instructions for compiling from source are identical to the :doc:`chipKIT
Max32 <max32>` except that ``PLATFORM=CROSSCHASM_BTLE_C5`` instead of ``CHIPKIT``.

If you will not be using the avrdude bootloader and will be flashing directly
via ICSP, make sure to also compile with ``BOOTLOADER=0`` to enable the program
to run on bare metal.

USB
---

The micro-USB port on the board is used to send and receive OpenXC messages.

Debug Logging
-------------

Debug logging support on the BTLE_C5 is available on the USB port and is enabled by default
with the ``DEFAULT_LOGGING_OUTPUT="USB"`` build option. UART logging is not available on
the BTLE_C5 device


LED Lights
-----------

The BTLE_C5 has 2 user controllable LEDs. When CAN activity is detected, the green
LED will be enabled. When USB or bluetooth low energy is connected, the blue LED will be enabled. If CAN is silent the red LED will be enabled. All LEDs will be turned off when sleep mode is entered
