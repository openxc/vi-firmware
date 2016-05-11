CrossChasm C5 Interfaces
========================
CrossChasm C5 family of OBD interfaces include versions with different communication radios
such as the Bluetooth Classic, :doc:`Bluetooth Low Energy</platforms/crosschasm-c5-ble>` and :doc:`Cellular(GPRS)</platforms/crosschasm-c5-cellular>`. Most things are common
between the different devices and are described on this page. This page covers the
Bluetooth Classic version or ``PLATFORM=CROSSCHASM_C5_BT``. See the particular BLE and Cellular pages
for specific differences.

CrossChasm's C5 OBD interface is compatible with the OpenXC VI
firmware. To build for one of the C5s, compile with one of the flags: 
``PLATFORM=CROSSCHASM_C5_BT``, ``PLATFORM=CROSSCHASM_C5_BLE`` or ``PLATFORM=CROSSCHASM_C5_CELLULAR``.

.. note::

   The old ``CROSSCHASM_C5`` Platform has been renamed to ``CROSSCHASM_C5_BT`` and 
   the fabric shortcut ``c5`` has been updated to ``c5bt``. Both old variables are 
   aliased to the new BT forms.

   
All C5s come pre-loaded with the correct bootloader, so you don't need any additional
hardware to load the OpenXC firmware. You do have flash an appropriate firmware for your
application.

The C5 connects to the `CAN1 bus pins
<http://openxcplatform.com/vehicle-interface/#obd-pins>`_ on the OBD-II
connector.

.. note::

   CAN2 pins ``CROSSCHASM_C5_BLE`` are mapped to pin 3 and pin 11 of the OBD-II 
   connector.

Flashing a Pre-compiled Firmware
--------------------------------

Assuming your C5 has the :ref:`bootloader <bootloader>` already flashed, follow
these :download:`instructions </_static/QuickStart guide to using C5 Hardware and OpenXC.pdf>`.

.. _bootloader:

Bootloader
----------

The C5 can be flashed with the same `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_, as the chipKIT.

The OpenXC fork of the bootloader (the previous link) defines a `CROSSCHASM_C5` configuration that
exposes a CDC/ACM serial port function over USB. Once the bootloader is flashed, there
is a 5 second window when the unit powers on when it will accept bootloader
commands.

In Linux and OS X it will show up as something like `/dev/ACM0`, and you can treat this
just as if it were a serial device.

In Windows, you will need to install the `stk500v2.inf
<https://raw.github.com/openxc/PIC32-avrdude-bootloader/master/Stk500v2.inf>`_
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
Max32 <max32>` except that ``PLATFORM=CROSSCHASM_C5_BT``, ``PLATFORM=CROSSCHASM_C5_BLE`` 
or ``PLATFORM=CROSSCHASM_C5_CELLULAR`` instead of ``CHIPKIT``.

If you will not be using the avrdude bootloader and will be flashing directly
via ICSP, make sure to also compile with ``BOOTLOADER=0`` to enable the program
to run on bare metal.

USB
---

The micro-USB port on the board is used to send and receive OpenXC messages.

UART
----

On the CROSSCHASM_C5_BT, ``UART1A`` is used for OpenXC output at the 230000 baud rate.
Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low by the
receiving device before data will be sent.

UART data is sent only if pin 0.58 (or PORTB BIT 4, RB4) is pulled high (to
5v). If you are using a Bluetooth module like the `BlueSMiRF
<https://www.sparkfun.com/products/10269>`_ from SparkFun, you need to hard-wire
5v into this pin to actually enabling UART. To disable UART, pull this pin low
or leave it floating.

Mass Storage Device
-------------------

The ``CROSSCHASM_C5_BT`` and ``CROSSCHASM_C5_CELLULAR`` are equipped with an SD card
reader. If enabled, device messages (except debug messages) are logged to the SD card. See
:doc:`MSD</advanced/msd>` for more info.

Real Time Clock
----------------
The C5 family of devices have a low power RTC chip that is connected to the PIC32 over the I2C
bus. The RTC enables timestamping of vehicle messages at the time of generation. Timestamps
are generated with millisecond resolution. See :doc:`RTC</advanced/rtc>` for more info.

Debug Logging
-------------

In most cases the logging provided via USB is sufficient, but if you are doing
low-level development and need the simpler UART interface, you can enable it
with the ``DEFAULT_LOGGING_OUTPUT="UART"`` build option.

On the C5, logging is on UART3A at 115200 baud (if the firmware was compiled
with ``DEBUG=1``).

.. note::

   If ``MSD_ENABLE=1`` debug logging is not available as these pins are shared with 
   the RTC for time stamping.

LED Lights
-----------

The C5 has 2 user controllable LEDs. When CAN activity is detected, the green
LED will be enabled. When USB or Bluetooth is connected, the blue LED will be
enabled.


