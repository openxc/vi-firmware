=================================
OpenXC CAN Translator
=================================

.. image:: /_static/logo.png

:Version: 3.0-dev
:Web: http://openxcplatform.com
:Documentation: http://openxcplatform.com/cantranslator
:Source: http://github.com/openxc/cantranslator
:Keywords: vehicle, openxc, embedded

--

The CAN translation module code runs on an Arduino-compatible microcontroller
connected to one or more CAN buses. It receives either all CAN messages or a
filtered subset, performs any unit conversion or factoring required and outputs
a generic version to a USB interface.

For more documentation, see the `vehicle interface`_ section on the `OpenXC
website`_ or the `CAN translator documentation`_.

.. _`OpenXC website`: http://openxcplatform.com
.. _`vehicle interface`: http://openxcplatform.com/vehicle-interface/firmware.html
.. _`CAN translator documentation`: http://openxcplatform.com/cantranslator

License
=======

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

The Windows driver for the CAN translator and its installer available in
`conf/windows-driver` is licensed under the GPL. The installer for the driver
is licensed under the LGPL.
