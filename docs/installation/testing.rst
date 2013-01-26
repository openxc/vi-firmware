=========
Testing
=========

Windows USB Device Driver
=========================

On Windows, a driver is required to use the CAN translator's USB interface. A
driver is available in the `conf/windows-driver
<https://github.com/openxc/cantranslator/tree/master/conf/windows-driver>`_
folder. The driver supports both 32- and 64-bit Windows. The driver is generated
using the `libusb-win32 <http://sourceforge.net/apps/trac/libusb-win32/wiki>`_
project.

Python Library
==============

The `OpenXC Python library`_, in particular the `openxc-dashboard` tool, is
useful for testing the CAN translator with a regular computer, to verify the
data received from a vehicle before introducing an Android device. Documentation
for this tool (and the list of required dependencies) is available on the OpenXC
`vehicle interface testing`_ page.

.. _`vehicle interface testing`: http://openxcplatform.com/vehicle-interface/testing.html
.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

Emulator
=========

The repository includes a rudimentary CAN bus emulator:

::

    $ make clean
    $ make emulator

The emulator generates fakes values for many OpenXC signals and sends
them over USB as if it were plugged into a live CAN bus.

Test Suite
===========

The non-embedded platform specific code in this repository includes a unit test
suite. It's a good idea to run the test suite before committing any changes to
the git repository.

Dependencies
------------

The test suite uses the `check <http://check.sourceforge.net>`_ library.

Ubuntu
~~~~~~~~~~

.. code-block:: sh

    $ sudo apt-get install check

OS X
~~~~~~~~~~

Install `Homebrew`_, then ``check``:

.. code-block:: sh

    $ brew install check

Arch Linux
~~~~~~~~~~

.. code-block:: sh

    $ sudo pacman -S check

Running the Suite
-----------------

.. code-block:: sh

    cantranslator/src $ make clean && make test -s

.. _`Homebrew`: http://mxcl.github.com/homebrew/

Debugging information
=====================

Viewing Debugging data
----------------------

To view debugging information, first compile the firmware with the
debugging flag:

.. code-block:: sh

    $ make clean
    $ DEBUG=1 make
    $ make flash

When compiled with ``DEBUG=1``, two things happen:

- Debug symbols are available in the .elf file generated in the ``build``
  directory.
- Log messages will be output over a UART port (no hardware flow control is
  required)

    - On the chipKIT Max32, logging will be on UART2 (Pin 16 - Tx, Pin 17 - Rx)
      at 115200 baud.
    - On the Blueboard LPC1768H, logging will be on UART0 (Pin P0.3 - Rx, Pin
      P0.2 - Tx) at 115200 baud.

View this output using an FTDI cable and any of the many available serial
terminal monitoring programs, e.g. ``screen``, ``minicom``, etc.
