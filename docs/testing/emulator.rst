Emulator
--------

The repository includes a rudimentary CAN bus emulator:

::

    $ make clean
    $ make emulator -j4

The emulator generates fakes values for many OpenXC signals and sends
them over USB as if it were plugged into a live CAN bus.
