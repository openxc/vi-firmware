=================================
OpenXC Vehicle Interface Firmware
=================================

.. image:: /docs/_static/logo.png

:Version: 6.0-dev
:Web: http://openxcplatform.com
:Documentation: http://vi-firmware.openxcplatform.com
:Source: http://github.com/openxc/vi-firmware
:Keywords: vehicle, openxc, embedded

--

The OpenXC vehicle interface (VI) firmware runs on an Arduino-compatible
microcontroller connected to one or more CAN buses. It receives either all CAN
messages or a filtered subset, performs any unit conversion or factoring
required and outputs a generic version to a USB interface.

For more documentation, see the `vehicle interface`_ section on the `OpenXC
website`_ or the `vehicle interface documentation`_.

.. _`OpenXC website`: http://openxcplatform.com
.. _`vehicle interface`: http://openxcplatform.com/vehicle-interface/firmware.html
.. _`vehicle interface documentation`: http://vi-firmware.openxcplatform.com

Quick Start
===========

For the full build instructions, see the `documentation
<http://vi-firmware.openxcplatform.com/en/latest/installation/installation.html>`_.

The basics to compile the firmware from source:

Clone the `vi-firmware <https://github.com/openxc/vi-firmware>`_ repository
(don't download the ZIP file, it won't work):

  .. code-block:: sh

    $ git clone https://github.com/openxc/vi-firmware

Run the ``bootstrap.sh`` script:

  .. code-block:: sh

    $ cd vi-firmware
    vi-firmware $ script/bootstrap.sh

Use the passthrough configuration to generate an implementation of ``signals.h``
and save it as ``signals.cpp``:

  .. code-block:: sh

    vi-firmware $ openxc-generate-firmware-code -m passthrough.json > signals.cpp

Compile it! By default this will compile for the chipKIT vehicle interface:

  .. code-block:: sh

    vi-firmware $ make

License
=======

Copyright (c) 2012-2013 Ford Motor Company

Licensed under the BSD license.

This repository includes links to other source code repositories (as git
submodules) that may be distributed under different licenses. See those
individual repositories for more details.
