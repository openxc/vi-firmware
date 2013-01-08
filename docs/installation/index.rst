============
Installation
============

If you've downloaded a pre-built binary for a specific vehicle, see the
:doc:`binary` section for instructions on how to flash your CAN
translator. Most users do not need to set up the full development described in
these docs.

Dependencies
============

In order to build the CAN translator firmware from source, you need a few
dependencies:

* ``cantranslator`` :ref:`source` cloned with Git - not from a .zip file
* MPIDE
* Digilent's USB and CAN libraries for the chipKIT
* FTDI driver
* Mini-USB cable

If instead of the chipKIT, you are compiling for the Blueboard (based on the
NXP LPC1768/69), instead of MPIDE you will need:

* ``gcc-arm-none-eabi`` toolchain
* ``openocd``
* JTAG programmer compatible with ``openocd`` - we've tested the Olimex
  ARM-OCD-USB programmer.

.. _source:

Source Code
-----------

Clone the repository from GitHub:

.. code-block:: sh

   $ git clone https://github.com/openxc/cantranslator

Some of the library dependencies are included in this repository as git
submodules, so before you go further run:

.. code-block:: sh

    $ git submodule update --init

If this doesn't print out anything or gives you an error, make sure you cloned
this repository from GitHub with git and that you didn't download a zip file.
The zip file is missing all of the git metadata, so submodules will not work.

MPIDE
-----

Building the source for the CAN translator for the chipKIT microcontroller
requires `MPIDE <https://github.com/chipKIT32/chipKIT32-MAX/downloads>`_ (the
development environment and compiler toolchain for chipKIT provided by
Digilent). Installing MPIDE can be a bit quirky on some platforms, so if you're
having trouble take a look at the `installation guide for MPIDE
<http://chipkit.org/wiki/index.php?title=MPIDE_Installation>`_.

Although we just installed MPIDE, building via the GUI is **not supported**. We
use GNU Make to compile and upload code to the device. You still need to
download and install MPIDE, as it contains the PIC32 compiler.

You need to set an environment variable (e.g. in ``$HOME/.bashrc``) to
let the project know where you installed MPIDE (make sure to change
these defaults if your system is different!):

.. code-block:: sh

    # Path to the extracted MPIDE folder (this is correct for OS X)
    export MPIDE_DIR=/Applications/Mpide.app/Contents/Resources/Java

Remember that if you use ``export``, the environment variables are only
set in the terminal that you run the commands. If you want them active
in all terminals (and you probably do), you need to add these
``export ...`` lines to the file ``~/.bashrc`` (in Linux) or
``~/.bash_profile`` (in OS X) and start a new terminal.

Digilent / Microchip Libraries
------------------------------

It also requires some libraries from Microchip that we are unfortunately unable
to include or link to as a submodule from the source because of licensing
issues:

-  Microchip USB device library (download DSD-0000318 from the bottom of
   the `Network Shield
   page <http://digilentinc.com/Products/Detail.cfm?NavPath=2,719,943&Prod=CHIPKIT-NETWORK-SHIELD>`_)
-  Microchip CAN library (included in the same DSD-0000318 package as
   the USB device library)

You can read and accept Microchip's license and download both libraries on the
`Digilent download page
<http://digilentinc.com/Agreement.cfm?DocID=DSD-0000318>`_.

Once you've downloaded the .zip file, extract it into the ``libs``
directory in this project. It should look like this:

.. code-block:: sh

    - /Users/me/projects/cantranslator/
    ---- libs/
    -------- chipKITUSBDevice/
             chipKitCAN/
            ... other libraries

FTDI Driver
-----------

If you're using Mac OS X or Windows, make sure to install the FTDI driver that
comes with the MPIDE download. The chipKIT uses a different FTDI chip than the
Arduino, so even if you've used the Arduino before, you still need to install
this driver.

OpenOCD
--------

Arch Linux
~~~~~~~~~~

.. code-block:: sh

    $ pacman -S openocd

OS X
~~~~

Install `Homebrew`_. Then:

.. code-block:: sh

    $ brew install libftdi libusb

Download the OpenOCD source distribution and build manually:

.. code-block:: sh

    $ ./configure --enable-ft2232_libftdi
    $ make
    $ sudo make install

Edit
``/System/Library/Extensions/FTDIUSBSerialDriver.kext/Contents/Info.plist``
and remove the Olimex sections, then reload the module:

.. code-block:: sh

    $ sudo kextunload /System/Library/Extensions/FTDIUSBSerialDriver.kext/
    $ sudo kextload /System/Library/Extensions/FTDIUSBSerialDriver.kext/

GCC for ARM Toolchain
---------------------

Arch Linux
~~~~~~~~~~

Download and install the ``gcc-arm-none-eabi`` package from AUR.

OS X
~~~~

Install `Homebrew`_. Then:

.. code-block:: sh

    $ brew tap PX4/homebrew-px4
    $ brew install gcc-arm-none-eabi

Wait a looooong time.

.. _`Homebrew`: http://mxcl.github.com/homebrew/
