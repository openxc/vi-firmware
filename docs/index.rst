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


Setup
======

.. toctree::
    :maxdepth: 2

    installation
    definitions
    compiling
    testing

Supported Embedded Platforms
=============================

The firmware supports :doc:`multiple microcontrollers </platforms/platforms>`.

.. toctree::
    :maxdepth: 2

    platforms/platforms

Advanced Firmware Features
==========================

.. toctree::
    :maxdepth: 1
    :glob:

    advanced/*

Output Interfaces & Format
===========================

The OpenXC message format is specified and versioned separately from any of the
individual OpenXC interfaces or libraries, in the `OpenXC Message Format
<https://github.com/openxc/openxc-message-format>`_ repository.

.. toctree::
    :maxdepth: 1
    :glob:

    output/*

Contributing
==============

Please see our `Contributing Guide`_.

.. _`Contributing Guide`: https://github.com/openxc/vi-firmware/blob/master/CONTRIBUTING.mkd

Mailing list
------------

For discussions about the usage, development, and future of OpenXC, please join
the `OpenXC mailing list`_.

.. _`OpenXC mailing list`: http://groups.google.com/group/openxc

Bug tracker
------------

If you have any suggestions, bug reports or annoyances please report them
to our issue tracker at http://github.com/openxc/vi-firmware/issues/

.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

Related Projects
================

Python Library
----------------------

The `OpenXC Python library`_, in particular the `openxc-dashboard` tool, is
useful for testing the VI with a regular computer, to verify the
data received from a vehicle before introducing an Android device. Documentation
for this tool (and the list of required dependencies) is available on the OpenXC
`vehicle interface testing`_ page.

.. _`vehicle interface testing`: http://openxcplatform.com/vehicle-interface/testing.html
.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

Android Library
----------------------

The `OpenXC Android library`_ is the primary entry point for new OpenXC
developers. More information on this library is available at in the
`applications`_ section of the `OpenXC website`_.

.. _`applications`: http://openxcplatform.com/android/index.html
.. _`OpenXC Android library`: https://github.com/openxc/openxc-android
.. _`OpenXC website`: http://openxcplatform.com

License
=======

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

This software depends on other open source projects, and a binary distribution
may contain code covered by :doc:`other licenses <license-disclosure>`.
