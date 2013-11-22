=================================
OpenXC Vehicle Interface Firmware
=================================

.. image:: /_static/logo.png

:Version: 5.1.3-dev
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

.. toctree::
    :maxdepth: 2

    getting-started

Configuring, Compiling and Testing
==================================

.. toctree::
    :maxdepth: 1

    config/config
    compile/compiling
    platforms/platforms
    testing

Advanced Reference
======================

.. toctree::
    :maxdepth: 1

    advanced/advanced
    output

Contributing
============

.. toctree::
    :maxdepth: 0

    contributing

License
-------

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

This software depends on other open source projects, and a binary distribution
may contain code covered by :doc:`other licenses <license-disclosure>`.
