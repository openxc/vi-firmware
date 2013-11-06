=================================
OpenXC Vehicle Interface Firmware
=================================

.. image:: /_static/logo.png

:Version: 5.1.2-dev
:Web: http://openxcplatform.com
:Documentation: http://vi-firmware.openxcplatform.com
:Source: http://github.com/openxc/vi-firmware

The OpenXC vehicle interface (VI) firmware runs on a microcontroller connected
to one or more CAN buses. It receives either all CAN messages or a filtered
subset, performs any unit conversion or factoring required and outputs a generic
version to a USB, Bluetooth or network interface.

.. WARNING::
   This portion of the site covers advanced topics such as writing and compiling
   your own firmware from source. These steps are NOT required for flashing a
   pre-compiled binary firmware.

   If you've downloaded a pre-built binary firmware for your car, locate your VI
   in the `list of supported interfaces
   <http://openxcplatform.com/vehicle-interface/hardware.html>`_ to find
   instructions for programming it. You don't need anything from the VI firmware
   documentation itself - most users don't need anything in this documentation.
   Here be dragons!

Getting Started
===============

.. toctree::
    :maxdepth: 2

    config/getting-started.rst
    config/getting-started-compiling.rst

Configuring, Compiling and Testing
==================================

.. toctree::
    :maxdepth: 2
    :glob:

    platforms/platforms
    config/config
    compiling
    testing

Advanced Documentation
======================

.. toctree::
    :maxdepth: 2
    :glob:

    advanced/advanced
    output/output

Contributing
============

.. toctree::
    :maxdepth: 0
    :glob:

    contributing

License
-------

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

This software depends on other open source projects, and a binary distribution
may contain code covered by :doc:`other licenses <license-disclosure>`.
