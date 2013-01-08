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

The ``check`` library is available in the AUR in both 64- and 32-bit versions.
Because we run on a 32-bit embedded platform, we depend on the 32-bit version of
``check``. In the AUR this package is named ``lib32-check``.

Running the Suite
-----------------

.. code-block:: sh

    cantranslator/src $ make clean && make test

.. _`Homebrew`: http://mxcl.github.com/homebrew/
