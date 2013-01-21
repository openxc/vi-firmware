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
debugging flag:

.. code-block:: sh

    $ make clean
    $ DEBUG=1 make
    $ make flash

When compiled with ``DEBUG=1``, two things happen:

* Debug symbols are available in the .elf file generated in the ``build``
  directory.
* Log messages will be output over a UART port (no hardware flow control is
  required)
    * On the chipKIT Max32, logging will be on UART2 (Pin 17 - Rx, Pin 18 - Tx)
      at 115200 baud.
    * On the Blueboard LPC1768H, logging will be on UART0 (Pin P0.3 - Rx, Pin
      P0.2 - Tx) at 115200 baud.

View this output using an FTDI cable and any of the many available serial
terminal monitoring programs, e.g. ``screen``, ``minicom``, etc.
