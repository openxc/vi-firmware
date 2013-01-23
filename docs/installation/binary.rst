==============================
Flashing a Pre-compiled Binary
==============================

Updates to the CAN translator firmware may be distributed as
pre-compiled binaries, e.g. if they are distributed by an OEM who does
not wish to make the CAN signals public. If that's the case for your
vehicle, you will have a ``.hex`` file and can use the
`upload\_hex.sh <https://github.com/openxc/cantranslator/blob/master/script/upload_hex.sh>`_
script to update your device.

Prerequisites
=============

In order to flash a CAN translator, you need:

* AVR programmer
* FTDI Driver
* Mini-USB cable

FTDI Driver
-----------

If you are using Windows or OS X, you need to install the FTDI
driver. If you didn't need to install MPIDE, you can download the driver
separately from `FTDI <http://www.ftdichip.com/Drivers/VCP.htm>`_.

AVR Programmer
--------------

In order to program the CAN translator, you need an AVR programmer tool. There
are a number of options that will work.

*With MPIDE*

If you have `MPIDE`_ installed, that already includes a version of avrdude. You
need to set the ``MPIDE_DIR`` environment variable in your terminal to point to
the folder where you installed MPIDE. Once set, you should be able to use
`upload\_hex.sh <https://github.com/openxc/cantranslator/blob/master/script/upload_hex.sh>`_.

*Without MPIDE*

If you do not already have `MPIDE`_ installed (and that's fine, you don't really
need it), you can install a programmer seprately:

- Linux - Look for ``avrdude`` in your distribution's package manager.
- OS X - Install ``avrdude`` with `Homebrew`_.
- Windows
   - If you prefer a GUI, install `WinAVR <http://winavr.sourceforge.net/>`_
   - If you prefer command line, install `Cygwin <http://cygwin.com>`_ and
     `MPIDE`_, and follow the :doc:`installation` documentation to configure the MPIDE
     environment variables.

.. _`Homebrew`: http://mxcl.github.com/homebrew/

USB Cable
---------

You need to have the **mini-USB** port on the chipKIT connected to your computer
to upload a new firmware. This is different than the micro-USB port that you use
to read vehicle data - see the `device connections
<http://openxcplatform.com/vehicle-interface/index.html#connections>`_ section
of the `OpenXC website`_ to make sure you have the correct cable attached.

Flashing
========

Command Line
------------

Once you have ``avrdude`` installed, run the ``upload_hex.sh`` script with the
``.hex`` file you downloaded (Windows users see below, this script will *not*
work with the standard Windows command line or Powershell):

.. code-block:: sh

   $ script/upload_hex.sh <the firmware file you downloaded>.hex

If you have more than one virtual serial (COM) port active, you may need to
explicitly specific which port to use. Pass the port identified as the second
argument to the script, e.g. in Linux:

.. code-block:: sh

   $ script/upload_hex.sh <the firmware file you downloaded>.hex /dev/ttyUSB2

and in Windows, if you needed to use ``com4`` instead of the default ``com3``:

.. code-block:: sh

   $ script/upload_hex.sh <the firmware file you downloaded>.hex com4

**Windows notes:**

In Windows, this command will only work in Cygwin, not the standard
``cmd.exe`` or Powershell. If you have the ``sh.exe`` program installed by
some other means (e.g. you have Git Shell) then it will
actually work in Powershell, but you need to preface the command with ``sh``
(i.e. ``sh script/upload_hex.sh ...``).

If you get errors about ``$'\r': command not found`` then your Git
configuration added ``CRLF`` line endings and so you must run the script
like this:

.. code-block:: sh

   $ set -o igncr && export SHELLOPTS && script/upload_hex.sh <firmware you downloaded>.hex

WinAVR GUI
----------

The GUI should be straightforward, but additional documentation here would be a
wonderful contribution!

.. _`MPIDE`: https://github.com/chipKIT32/chipKIT32-MAX/downloads
.. _`OpenXC website`: http://openxcplatform.com
