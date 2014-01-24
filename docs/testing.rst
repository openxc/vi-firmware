=======
Testing
=======

Windows USB Device Driver
=========================

If you want to send and receive vehicle data in Windows via USB, you must
install the `VI Windows Driver <https://github.com/openxc/vi-windows-driver>`.

Python Library
==============

The `OpenXC Python library`_, in particular the `openxc-dashboard` tool, is
useful for testing the VI with a regular computer, to verify the
data received from a vehicle before introducing an Android device. A quick
"smoke test" using the Python tools is described in the `Getting Started Guide
<http://openxcplatform.com/python/getting-started.html>`_ for Python developers
at the OpenXC website.

Keep in mind when bench testing - the VI will go into a low power suspend mode
if no CAN activity has been detected for 30 seconds. If you compile with the
`DEBUG` flag, it will not suspend.

.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

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
- Log messages will be output over a separate USB endpoint
  required) - see :doc:`/output` for details. You can optionally enable logging
  via UART with the ``UART_LOGGING`` flag, but there may be a performance
  hit - see the :doc:`/compile/makefile-opts`.

To view the logs via USB, you can use the command line tools from the OpenXC
Python library and supply the ``--log-mode`` flag - see the help text for any of
those tools for more information.

To view UART logs, you can use an FTDI cable and any of the many available
serial terminal monitoring programs, e.g. ``screen``, ``minicom``, etc.

Emulator
=========

The repository includes a rudimentary vehicle emulator version of the firmware:

::

    $ make clean
    $ make emulator

The emulator generates fakes values for many OpenXC signals and sends out
translated OpenXC messages as if it were plugged into a real vehicle.

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

    vi-firmware/src $ make clean && make test -s

.. _`Homebrew`: http://mxcl.github.com/homebrew/
