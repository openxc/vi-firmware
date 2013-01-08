Once this is set up, ``cd`` into the ``src`` subdirectory, build and
upload to the device.

::

    $ git submodule init && git submodule update
    $ make clean
    $ make
    $ make flash

If the flash command can't find your chipKIT, you may need to set the
``ARDUINO_PORT`` variable (if the serial emulator doesn't show up as
``/dev/ttyUSB*`` (in Linux) or ``/dev/tty.usbserial*`` (in Mac OS X)).

**Troubleshooting**

If the compilation didn't work:

-  Make sure the submodules are up to date - run
   ``git submodule update --init`` and then ``git status`` and make sure
   there are no modified files in the working directory.
-  Did you download the .zip file of the ``cantranslator`` project from
   GitHub? Try using git to clone the repository instead - the library
   dependencies are stored as git submodules and do not work when using
   the zip file.

LPC17xx
~~~~~~~

Support for the NXP LPC17xx, an ARM Cortex M3 microcontroller, is
experimental at the moment and the documentation is incomplete. We are
building successfully on the NGX Blueboard 1768-H using the Olimex
ARM-OCD-USB JTAG programmer.

Ubuntu
~~~~~~

Arch Linux
~~~~~~~~~~

::

    $ pacman -S openocd

Download and install from the AUR:

-  gcc-arm-none-eabi

OS X
~~~~

Install Homebrew. Then:

::

    $ brew install libftdi libusb
    $ brew tap PX4/homebrew-px4
    $ brew install gcc-arm-none-eabi

Wait a looooong time.

Download the OpenOCD source distribution and build manually:

::

    $ ./configure --enable-ft2232_libftdi
    $ make
    $ sudo make install

Edit
``/System/Library/Extensions/FTDIUSBSerialDriver.kext/Contents/Info.plist``
and remove the Olimex sections, then reload the module:

::

    $ sudo kextunload /System/Library/Extensions/FTDIUSBSerialDriver.kext/
    $ sudo kextload /System/Library/Extensions/FTDIUSBSerialDriver.kext/

Compiling
~~~~~~~~~

Once the dependencies are installed, attach a JTAG adapter to your
computer and the CAN translator, then compile and flash an LPC1768
Blueboard like so:

::

    $ make clean
    $ PLATFORM=LPC17XX make -j4
    $ PLATFORM=LPC17XX make flash

The config files in this repository assume your JTAG adapter is the
Olimex ARM-USB-OCD unit. If you have a different unit, change the first
line in ``conf/flash.cfg`` to the correct value.
