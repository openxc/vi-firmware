Digilent chipKIT Max32
=======================

To build for the chipKIT Max32, compile with the flag ``PLATFORM=CHIPKIT``. The
chipKIT is also the default platform, so the flag is optional.

Flashing a Pre-compiled Firmware
--------------------------------

These instructions assume your chipKIT is running the stock firmware, the
avrdude bootloader.

USB Cable
^^^^^^^^^

You need to have the **mini-USB** port on the chipKIT connected to your computer
to upload a new firmware. This is different than the micro-USB port that you use
to read vehicle data - see the `device connections
<http://openxcplatform.com/vehicle-interface/index.html#connections>`_ section
of the `OpenXC website`_ to make sure you have the correct cable attached.

Uploading Script
^^^^^^^^^^^^^^^^

Open a terminal run the ``upload_hex.sh`` script from the ``cantranslator``
directory, passing it the path to the ``.hex`` file you downloaded:

.. code-block:: sh

   $ cd cantranslator
   $ script/upload_hex.sh <firmware file you downloaded>.hex

The ``upload_hex.sh`` script attempts to install all required dependencies
automatically, and it is tested in Cygwin, OS X Mountain Lion, Ubuntu 12.04 and
Arch Linux - other operating systems may need to :ref:`install the dependencies
manually <manual-deps>`.

If you have more than one virtual serial (COM) port active, you may need to
explicitly specify which port to use. Pass the port name as the second argument
to the script, e.g. in Linux:

.. code-block:: sh

   $ script/upload_hex.sh <firmware file you downloaded>.hex /dev/ttyUSB2

and in Windows, e.g. if you needed to use ``com4`` instead of the default
``com3``:

.. code-block:: sh

   $ script/upload_hex.sh <firmware file you downloaded>.hex com4

Windows notes
"""""""""""""

In Windows, this command will only work in Cygwin, not the standard
``cmd.exe`` or Powershell.

If you get errors about ``$'\r': command not found`` then your Git configuration
added Windows-style ``CRLF`` line endings. Run this first to ignore the ``CR``:

.. code-block:: sh

   $ set -o igncr && export SHELLOPTS

.. _`MPIDE`: https://github.com/chipKIT32/chipKIT32-MAX/downloads
.. _`OpenXC website`: http://openxcplatform.com

.. _manual-deps:

Dependencies
^^^^^^^^^^^^

If the flashing script failed, you may need to install the dependencies
manually.

FTDI Driver
"""""""""""

If you are using Windows or OS X, you need to install the FTDI
driver. If you didn't need to install MPIDE, you can download the driver
separately from `FTDI <http://www.ftdichip.com/Drivers/VCP.htm>`_.

AVR Programmer
""""""""""""""
In order to program the CAN translator, you need to install an AVR programmer.
There are a number of free options that will work.

*With MPIDE*

If you have `MPIDE`_ installed, that already includes a version of avrdude. You
need to set the ``MPIDE_DIR`` environment variable in your terminal to point to
the folder where you installed MPIDE. Once set, you should be able to use
`upload\_hex.sh <https://github.com/openxc/cantranslator/blob/master/script/upload_hex.sh>`_.

*Without MPIDE*

If you do not already have `MPIDE`_ installed (and that's fine, you don't really
need it), you can install a programmer seprately:

- Linux - Look for ``avrdude`` in your distribution's package manager.
- OS X - Install ``avrdude`` with `Homebrew`_.
- Windows
   - Install `Cygwin <http://www.cygwin.com>`_ and `MPIDE`_, and follow the
     :doc:`/installation/installation` documentation to configure the MPIDE environment
     variables.

.. _`Homebrew`: http://mxcl.github.com/homebrew/

Bootloader
----------

The `PIC32 avrdude bootloader
<https://github.com/openxc/PIC32-avrdude-bootloader>`_ is tested and working and
allows flashing over USB with ``avrdude``. All stock chipKITs are programmed
with a compatible bootloader at the factory.

Compiling
---------

Once the :doc:`dependencies </installation/installation>` are installed, attach the chipKIT to
your computer with a mini-USB cable, ``cd`` into the ``src`` subdirectory, build
and upload to the device.

.. code-block:: sh

    $ make clean
    $ make
    $ make flash

If the flash command can't find your chipKIT, you may need to set the
``SERIAL_PORT`` variable if the serial emulator doesn't show up as
``/dev/ttyUSB*`` in Linux, ``/dev/tty.usbserial*`` in Mac OS X or ``com3`` in
Windows. For example, if the chipKIT shows up as ``/dev/ttyUSB4``:

.. code-block:: sh

    $ SERIAL_PORT=/dev/ttyUSB4 make flash

and if in Windows it appeared as COM4:

.. code-block:: sh

    $ SERIAL_PORT=com4 make flash

This build process assumes your chipKIT is running the
:doc:`avrdude bootloader </installation/bootloaders>` - all chipKITs come
programmed with a compatible bootloader by default.

IDE Support
-----------

It is possible to use an IDE like Eclipse to edit and compile the
project.

-  Follow the directions in the above "Installation" section.
-  Install Eclipse with the `CDT project <http://www.eclipse.org/cdt/>`_
-  In Eclipse, go to
   ``File -> Import -> C/C++ -> Existing Code as Makefile Project`` and
   then select the ``cantranslator/src`` folder.
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

An additional item to consider when using UART: typically you will want to rig
the chipKIT to be self-powered (either from an external power source or the
vehicle) if you're going to use UART for adding Bluetooth support. There's not
much point in being wireless if you still need power from USB.

In that case, move the power jumper from the 5v input on the Network Shield
to A0 (analog input 0). Instead of using 5v to power the board, the firmware can
use it to detect if USB is actually attached or not. The benefit of this is that
if you connect USB, then disconnect it, we can detect that in the firmware and
stop wasting time trying to send data over USB. This will dramatically increase
the throughput over UART.


Debug Logging
-------------

On the chipKIT Max32, logging will be on UART2 (Pin 16 - Tx, Pin 17 - Rx) at
115200 baud (if the firmware was compiled with ``DEBUG=1``).

LED Lights
-----------

The chipKIT has 1 user controllable LED. When CAN activity is detected, the LED
will be enabled (it's green).
