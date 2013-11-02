=================================
OpenXC Vehicle Interface Firmware
=================================

.. image:: /_static/logo.png

:Version: 5.1.2-dev
:Web: http://openxcplatform.com
:Documentation: http://vi-firmware.openxcplatform.com
:Source: http://github.com/openxc/vi-firmware

About
=====

The OpenXC vehicle interface (VI) firmware runs on an Arduino-compatible
microcontroller connected to one or more CAN buses. It receives either all CAN
messages or a filtered subset, performs any unit conversion or factoring
required and outputs a generic version to a USB interface.


Pre-built Binary Firmware
=========================

If you've downloaded a pre-built binary firmware for your car, locate your VI in
the `list of supported interfaces
<http://openxcplatform.com/vehicle-interface/hardware.html>`_ to find
instructions for programming it. You don't need anything from the VI firmware
documentation itself - most users don't need anything in this documentation.
Here be dragons!

Contributing
============

.. toctree::
    :maxdepth: 0
    :glob:

    contributing

Installation
============

.. toctree::
    :maxdepth: 2
    :glob:

    installation

Configuring, Compiling and Testing
==================================

.. toctree::
    :maxdepth: 2
    :glob:

    platforms/platforms
    definitions
    compiling
    testing

Advanced Documentation
======================

.. toctree::
    :maxdepth: 2
    :glob:

    advanced/advanced
    output/output


License
=======

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

This software depends on other open source projects, and a binary distribution
may contain code covered by :doc:`other licenses <license-disclosure>`.
