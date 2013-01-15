=========
Testing
=========

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

    cantranslator/src $ make clean && make test

.. _`Homebrew`: http://mxcl.github.com/homebrew/

Debugging information
=================

Viewing Debugging data
---------------------

To view debugging information, first compile the firmware with the
debugging flag.

.. code-block:: sh

    $ make clean
    $ DEBUG=1 make -j4
    $ make flash

To build for a Blueboard, add the Platform flag to your make command:

.. code-block:: sh

    $ DEBUG=1 PLATFORM=BLUEBOARD make -j4

Once the CAN Translator is built with active debugging, the data can
be viewed via an FTDI cable.  On the chipKit Max32, the debugging
information is on UART Port 2.  This port's RX is pin 17, and the TX
is pin 16.  Those are the only two wires that need be connected to
your FTDI cable.

Once connected, view the traffic on that serial port with the screen
command at 11520 baud.  On MacOS, the command is:

.. code-block:: sh

    $ screen /dev/tty.usbserial________  115200

Be sure to substitue the actual serial port identifier.
