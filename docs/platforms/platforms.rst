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

No data received over USB?
    If you have UART enabled for a while, then disconnect the UART receiver
    (i.e. pull the status pin low and stop touching RTS/CTS), it can cause the
    firmware to block trying to write data to UART. Power cycle the board or
    leave the UART receiver attached even if nobody is reading data (i.e. keep
    the CTS/RTS lines active).

USB data arriving in bursts?
    Are you also reading data over UART, or do you have something pulling the
    UART connect pin high? It's not always possible to read both USB and UART at
    full data rates at the same time.
