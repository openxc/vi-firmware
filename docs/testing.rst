=========
Testing
=========

Emulator
=========

The repository includes a rudimentary CAN bus emulator:

::

    $ make clean
    $ make emulator -j4

The emulator generates fakes values for many OpenXC signals and sends
them over USB as if it were plugged into a live CAN bus.

Test Suite
===========

The non-embedded platform specific code in this repository includes a unit test
suite that uses the `check <http://check.sourceforge.net>`_ library. After
installing that library, run the test suite like so:

::

    cantranslator/ $ make clean && make test

The `check <http://check.sourceforge.net>`_ library can be installed in
Ubuntu Linux quite easily:

::

    $ sudo apt-get install check

and in Mac OS X, it's available through
`homebrew <http://mxcl.github.com/homebrew/>`_:

::

    $ brew install check

