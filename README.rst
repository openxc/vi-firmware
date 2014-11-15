=================================
OpenXC Vehicle Interface Firmware
=================================

.. image:: /docs/_static/logo.png

:Version: 7.0.1-dev
:Web: http://openxcplatform.com
:Documentation: http://vi-firmware.openxcplatform.com
:Source: http://github.com/openxc/vi-firmware
:Keywords: vehicle, openxc, embedded

.. image:: https://travis-ci.org/openxc/vi-firmware.svg?branch=master
    :target: https://travis-ci.org/openxc/vi-firmware

.. image:: https://coveralls.io/repos/openxc/vi-firmware/badge.png?branch=master
    :target: https://coveralls.io/r/openxc/vi-firmware?branch=master

.. image:: https://readthedocs.org/projects/openxc-vehicle-interface-firmware/badge
    :target: http://vi-firmware.openxcplatform.com
    :alt: Documentation Status

The OpenXC vehicle interface (VI) firmware runs on an microcontroller connected
to one or more CAN buses. It receives either all CAN messages or a filtered
subset, performs any unit conversion or factoring required and outputs a generic
version to a USB interface.

For more documentation, see the `vehicle interface`_ section on the `OpenXC
website`_ or the `vehicle interface documentation`_.

.. _`OpenXC website`: http://openxcplatform.com
.. _`vehicle interface`: http://openxcplatform.com/vehicle-interface/firmware.html
.. _`vehicle interface documentation`: http://vi-firmware.openxcplatform.com

Installation
=============

For the full build instructions, see the `documentation
<http://vi-firmware.openxcplatform.com/en/latest/installation/installation.html>`_.

License
=======

Copyright (c) 2012-2014 Ford Motor Company

Licensed under the BSD license.

This repository includes links to other source code repositories (as git
submodules) that may be distributed under different licenses. See those
individual repositories for more details.
