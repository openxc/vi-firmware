=================================
OpenXC CAN Translator
=================================

.. image:: /_static/logo.png

:Version: 4.0.1
:Web: http://openxcplatform.com
:Documentation: http://vi-firmware.openxcplatform.com
:Source: http://github.com/openxc/cantranslator

About
=====

The CAN translation module code runs on an Arduino-compatible microcontroller
connected to one or more CAN buses. It receives either all CAN messages or a
filtered subset, performs any unit conversion or factoring required and outputs
a generic version to a USB interface.

The firmware supports :doc:`multiple microcontrollers </platforms/platforms>`.

Setup
======

.. toctree::
    :maxdepth: 2

    installation/installation
    platforms/platforms

Pre-built Binary
================

If you've downloaded a pre-built binary for a specific vehicle, see the
:doc:`/installation/binary` section for instructions on how to flash your CAN
translator. Most users do not need to set up the full development described in
these docs.

A Windows driver for the USB interface is available in the `conf/windows-driver
<https://github.com/openxc/cantranslator/tree/master/conf/windows-driver>`_
folder. The driver supports both 32- and 64-bit Windows. The driver is generated
using the `libusb-win32 <http://sourceforge.net/apps/trac/libusb-win32/wiki>`_
project.

.. _`OpenXC website`: http://openxcplatform.com
.. _`firmware section`: http://openxcplatform.com/vehicle-interface/firmware.html

CAN Message Definition
======================

.. toctree::
    :maxdepth: 1
    :glob:

    definitions/*

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

.. _`Contributing Guide`: https://github.com/openxc/cantranslator/blob/master/CONTRIBUTING.mkd

Mailing list
------------

For discussions about the usage, development, and future of OpenXC, please join
the `OpenXC mailing list`_.

.. _`OpenXC mailing list`: http://groups.google.com/group/openxc

Bug tracker
------------

If you have any suggestions, bug reports or annoyances please report them
to our issue tracker at http://github.com/openxc/cantranslator/issues/

.. _`OpenXC Python library`: https://github.com/openxc/openxc-python

Related Projects
================

Python Library
----------------------

The `OpenXC Python library`_, in particular the `openxc-dashboard` tool, is
useful for testing the CAN translator with a regular computer, to verify the
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

License
=======

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.
