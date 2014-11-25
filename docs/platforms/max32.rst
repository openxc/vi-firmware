Digilent chipKIT Max32
=======================

To build for the `chipKIT-based Vehicle Interface
<http://chipkit-vi.openxcplatform.com/>`_, compile with the flag
``PLATFORM=CHIPKIT``. The chipKIT is also the default platform, so the flag is
optional.

The chipKIT VI supports up to 2 of the CAN1, CAN2-1 or CAN2-2 buses
simultaneously.

For more details, see the `chipKIT's documentation
<http://chipkit-vi.openxcplatform.com>`_.

For instructions on flashing a new firmware version to the chipKIT, see the
`chipKIT firmware programming documentation
<http://chipkit-vi.openxcplatform.com/firmware/programming.html>`_.

USB
---

The micro-USB port on the Digilent Network Shield is used to send and receive
OpenXC messages. The mini-USB cable on the Max32 itself is only used for
re-programming.

UART
----

On the chipKIT, ``UART1A`` is used for OpenXC output at the 230000 baud rate.
Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low by the
receiving device before data will be sent. There are a few tricky things to
watch out for with UART (i.e. Bluetooth) output on the chipKIT, so make sure to
read this entire section.

``UART1A`` is also used by the USB-Serial connection, so in order to flash the
PIC32, the Tx/Rx lines must be disconnected. Ideally we could leave that UART
interface for debugging, but there are conflicts with all other exposed UART
interfaces when using flow control.

- Pin 0 - ``U1ARX``, connect this to the TX line of the receiver.
- Pin 1 - ``U1ATX``, connect this to the RX line of the receiver.
- Pin 18 - ``U1ARTS``, connect this to the CTS line of the receiver.
- Pin 19 - ``U1ACTS``, connect this to the RTS line of the receiver.

UART data is sent only if pin A1 is pulled low (to ground). If you are using a
Bluetooth module like the `BlueSMiRF <https://www.sparkfun.com/products/10269>`_
from SparkFun, you need to hard-wire GND into this pin to actually enabling
UART. To disable UART, pull A1 high (hard-wire to 5v) or leave it floating.

No data received over UART (i.e. Bluetooth)?
    If you are powering the device via USB but not also reading data via USB, it
    may be blocked waiting to send data. Try unplugging the USB connection and
    powering the device via the OBD connector.

Debug Logging
-------------

In most cases the logging provided via USB is sufficient, but if you are doing
low-level development and need the simpler UART interface, you can enable it
with the ``DEFAULT_LOGGING_OUTPUT="UART"`` build option, but be aware that UART
logging will dramatically decrease the performance of the VI.

On the chipKIT Max32, UART logging will be on UART2 (Pin 16 - Tx, Pin 17 - Rx)
at 115200 baud.

LED Lights
-----------

The chipKIT has 1 user controllable LED. When CAN activity is detected, the LED
will be enabled (it's green).

Compiling and Flashing
----------------------

Attach the chipKIT to your computer with a mini-USB cable, ``cd`` into the
``src`` subdirectory, build and upload to the device.

.. code-block:: sh

    vi-firmware/src/ $ fab chipkit build
    vi-firmware/src/ $ make flash

If the flash command can't find your chipKIT, you may need to set the
``SERIAL_PORT`` variable if the serial emulator doesn't show up as
``/dev/ttyUSB*`` in Linux, ``/dev/tty.usbserial*`` in Mac OS X or ``com3`` in
Windows. For example, if the chipKIT shows up as ``/dev/ttyUSB4``:

.. code-block:: sh

    $ SERIAL_PORT=/dev/ttyUSB4 make flash

and if in Windows it appeared as COM4:

.. code-block:: sh

    $ SERIAL_PORT=com4 make flash

IDE Support
-----------

It is possible to use an IDE like Eclipse to edit and compile the
project.

-  Follow the directions in the above "Installation" section.
-  Install Eclipse with the `CDT project <http://www.eclipse.org/cdt/>`_
-  In Eclipse, go to
   ``File -> Import -> C/C++ -> Existing Code as Makefile Project`` and
   then select the ``vi-firmware/src`` folder.
-  In the project's properties, under
   ``C/C++ General -> Paths and Symbols``, add these to the include
   paths for C and C++:

   -  ``${MPIDE_DIR}/hardware/pic32/compiler/pic32-tools/pic32mx/include``
   -  ``${MPIDE_DIR}/hardware/pic32/cores/pic32``
   -  ``/src/libs/CDL/LPC17xxLib/inc`` (add as a "workspace
      path")
   -  ``/src/libs/chipKITCAN`` (add as a "workspace path")
   -  ``/src/libs/chipKITUSBDevice`` (add as a "workspace
      path")
   -  ``/src/libs/chipKITUSBDevice/utility`` (add as a
      "workspace path")
   -  ``/src/libs/chipKITEthernet`` (add as a "workspace
      path")
   -  ``/usr/include`` (only if you want to use the test suite, which
      requires the ``check`` C library)

-  In the same section under Symbols, if you are building for the
   chipKIT define a symbol with the name ``__PIC32__``
-  In the project folder listing, select
   ``Resource Configurations -> Exclude from   Build`` for these
   folders:

   -  ``src/libs``
   -  ``build``

If you didn't set up the environment variables from the ``Installation``
section (e.g. ``MPIDE_HOME``), you can also do that from within Eclipse
in ``C/C++`` project settings.

There will still be some errors in the Eclipse problem detection, e.g.
it doesn't seem to pick up on the GCC ``__builtin_*`` functions, and
some of the chipKIT libraries are finicky. This won't have an effect on
the actual build process, just the error reporting.

Bootloader
----------

All stock chipKITs are programmed with a compatible bootloader at the factory.
The `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_ is also tested and
working and allows flashing over USB with ``avrdude``.
