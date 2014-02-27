===================
Supported Platforms
===================

For information on how to add support for another platform, see the :doc:`board
support </advanced/boardsupport>` section.

.. toctree::
    :maxdepth: 1

    ford
    blueboard
    max32
    crosschasm-c5

Troubleshooting PIC32 Boards
----------------------------

No data received over UART (i.e. Bluetooth)?
    If you are powering the device via USB but not also reading data via USB, it
    may be blocked waiting to send data. See the documentation for your specific
    platform for information on how to enable VBUS detection, or power the
    device via the OBD pins.
