====================
Building from Source
====================

The code base currently supports two embeddedd platforms - the chipKIT (based on
the Microchip PIC32) and the Blueboard (based on the NXP LPC1768/69). Before you
can compile, you will need to :doc:`define your CAN messages </definitions/definitions>`.

The build process works with Linux (tested in Arch Linux and Ubuntu), OS X and
Cygwin in Windows.

.. note::

   When running ``make`` to compile, try adding the ``-j4`` flag to build jobs
   in parallel - the speedup can be quite dramatic.

chipKIT Max32
=============

Once the :doc:`dependencies <installation>` are installed, attach the chipKIT to
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


Troubleshooting
---------------

If the compilation didn't work:

-  Make sure the submodules are up to date - run
   ``git submodule update --init`` and then ``git status`` and make sure
   there are no modified files in the working directory.
-  Did you download the .zip file of the ``cantranslator`` project from
   GitHub? Try using git to clone the repository instead - the library
   dependencies are stored as git submodules and do not work when using
   the zip file.

If you get a lot of errors about ``undefined refernece to getSignals()'`` and
other functions, you need to make sure you defined your CAN messages - read
through :doc:`/definitions/definitions` before trying to compile.

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

NGX Blueboard
==============

Support for the NXP LPC17xx, an ARM Cortex M3 microcontroller, is
experimental at the moment and the documentation is incomplete. We are
building successfully on the NGX Blueboard 1768-H using the Olimex
ARM-OCD-USB JTAG programmer.

Once the :doc:`dependencies <installation>` are installed, attach a JTAG adapter to
your computer and the CAN translator, then compile and flash:

.. code-block:: sh

    $ make clean
    $ PLATFORM=BLUEBOARD make -j4
    $ PLATFORM=BLUEBOARD make flash

The config files in this repository assume your JTAG adapter is the
Olimex ARM-USB-OCD unit. If you have a different unit, change the first
line in ``conf/flash.cfg`` to the correct value.

