Dependencies
============

In order to build the CAN translator firmware from source, you need a few
dependencies:

* Git
* ``cantranslator`` :ref:`source code <source>` cloned with Git - not from a .zip file
* `OpenXC Python library <http://python.openxcplatform.com>`_
* :ref:`MPIDE <mpidedep>`
* Digilent's USB and CAN :ref:`libraries for the chipKIT <chipkit-libs>`
* :ref:`FTDI driver <ftdi>`
* Mini-USB cable

If instead of the chipKIT, you are compiling for the Blueboard (based on the
NXP LPC1768/69), instead of MPIDE you will need:

* :ref:`GCC for ARM <gcc-arm>` toolchain
* :ref:`OpenOCD <openocddep>`
* JTAG programmer compatible with ``openocd`` - we've tested the Olimex
  ARM-OCD-USB programmer.

The easiest way to install these dependencies is to use the
`script/bootstrap.sh
<https://github.com/openxc/cantranslator/blob/master/script/bootstrap.sh>`_
script in the ``cantranslator`` repository. Run the script in Linux, Cygwin in
Windows or OS X and if there are no errors you should be ready to go:

.. code-block:: sh

  $ script/bootstrap.sh

If there are errors, continue reading in this section to install whatever piece
failed manually.

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

.. _mpidedep:

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

.. _chipkit-libs:

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

.. _ftdi:

FTDI Driver
-----------

If you're using Mac OS X or Windows, make sure to install the FTDI driver that
comes with the MPIDE download. The chipKIT uses a different FTDI chip than the
Arduino, so even if you've used the Arduino before, you still need to install
this driver.

.. _openocddep:

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
    $ brew install --enable-ft2232_libftdi openocd

Remove the Olimex sections from the FTDI kernel module, and then reload it:

.. code-block:: sh

    $ sudo sed -i "" -e "/Olimex OpenOCD JTAG A/{N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;d;}" /System/Library/Extensions/FTDIUSBSerialDriver.kext/Contents/Info.plist
    $ sudo kextunload /System/Library/Extensions/FTDIUSBSerialDriver.kext/
    $ sudo kextload /System/Library/Extensions/FTDIUSBSerialDriver.kext/

.. _gcc-arm:

GCC for ARM Toolchain
---------------------

Download the binary version of the toolchain for your platform (Linux, OS X or
Windows) from this `Launchpad site <https://launchpad.net/gcc-arm-embedded>`_.

Arch Linux
~~~~~~~~~~

In Arch Linux you can alternatively install the ``gcc-arm-none-eabi`` package
from the AUR.

.. _`Homebrew`: http://mxcl.github.com/homebrew/
