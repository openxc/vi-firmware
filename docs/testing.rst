=======
Testing
=======

Windows USB Device Driver
=========================

If you want to send and receive vehicle data in Windows via USB, you must
install the `VI Windows Driver <https://github.com/openxc/vi-windows-driver>`_.

Python Library
==============

The `OpenXC Python library`_, in particular the ``openxc-dashboard`` tool, is
useful for testing a VI. A quick "smoke test" using the Python tools is
described in the `Getting Started Guide
<http://openxcplatform.com/python/getting-started.html>`_ for Python developers
at the OpenXC website.

Keep in mind when bench testing that the VI will suspend if no CAN bus activity
is detected. Compiled with ``DEFAULT_POWER_MANAGEMENT=ALWAYS_ON`` to stop this
behavior, but don't leave it plugged into your car with power management off.

.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

Debugging
==========

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
  via UART with the ``DEFAULT_LOGGING_OUTPUT`` config option, but there may be a
  performance hit - see the :doc:`/compile/makefile-opts`.

To view the logs via USB, you can use the ``--log-mode`` flag with the Python
CLI tools. See the ``--help`` text for any of those tools for more information.

To view UART logs, you can use an FTDI cable and any of the many available
serial terminal monitoring programs, e.g. ``screen``, ``minicom``, etc. The pins
for this UART output are different for each board, so see the :doc:`platform
specific docs </platforms/platforms>`.

Test Suite
===========

The non-embedded platform specific code in this repository includes a unit test
suite. It's a good idea to run the test suite before committing any changes to
the git repository.

The test suite uses the `check <http://check.sourceforge.net>`_ library. It
should already be installed if you used ``bootstrap.sh`` to set up your
development environment.

Running the Suite
-----------------

.. code-block:: sh

    vi-firmware/src $ make clean && make test

Functional Test Suite
=====================

For a complete functional test of the firmware code on the support hardware
platforms, the repository includes an automated script to program a VI and run a
self-test. The VIs are configured to self-receive every CAN message they send,
allowing the complete firmware stack to be tested without any physical CAN
configuration. The test script is written in Python and uses the OpenXC Python
library send commands to the VI, read responses and receive the test vehicle
data.

The functional test suite is currently supported on the:

* Ford Reference VI
* chipKIT VI

Running the Suite
-----------------

MAke sure you have the following connected to your development machine:

* Reference VI via USB
* USB JTAG programming connected to the refernece VI
* chipKIT micro-USB (for OpenXC interface)
* chipKIT mini-USB (for programming)

Then run the automated functional test suite:

.. code-block:: sh

    vi-firmware/ $ fab auto_functional_test

which will program each of the VIs and run the tests on them independently. The
process takes a couple of minutes.
