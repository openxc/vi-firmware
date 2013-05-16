===================
Supported Platforms
===================

The firmware supports compiling for the Microchip's PIC32 microcontroller and
NXP's LPC1768/69 (and possibly other ARM Cortex M3 micros).

The code base is expanding very organically from supporting only one board to
supporting multiple architectures and board variants. The strategy we have now:

* Switch between "platforms" with the PLATFORM flag - a platform encapsulates a
  micro architecture and a board variant.
* Implement different architecture-specific code in a subfolder for the micro
* Switch pins for board variants in in those same architecture-specific files
  (like in lights.cpp)

PIC32
=====

Two PIC32 boards are supported:

* :doc:`Digilent chipKIT Max32 <max32>`
* :doc:`FleetCarma Data Logger <fleetcarma>`

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


LPC176x
=======

* :doc:`NGX Blueboard LPC1768-H <blueboard>`
* :doc:`Ford Prototype Vehicle Interface <ford>`

.. toctree::
    :maxdepth: 2
    :glob:
    :hidden:

    *
