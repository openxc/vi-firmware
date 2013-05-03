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
* FleetCarma Data Logger

LPC176x
=======

* :doc:`NGX Blueboard LPC1768-H <blueboard>`
* :doc:`Ford Prototype Vehicle Interface <ford>`

.. toctree::
    :maxdepth: 2
    :glob:
    :hidden:

    *
