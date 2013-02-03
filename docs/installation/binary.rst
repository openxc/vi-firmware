==============================
Flashing a Pre-compiled Binary
==============================

Updates to the CAN translator firmware may be distributed as
pre-compiled binaries, e.g. if they are distributed by an OEM who does
not wish to make the CAN signals public. If that's the case for your
vehicle, you will have a ``.hex`` file and can use the
`upload\_hex.sh <https://github.com/openxc/cantranslator/blob/master/script/upload_hex.sh>`_
script to update your device.

Quick Start
============

Windows
-------

1. Install `Cygwin <http://www.cygwin.com>`_ and in the installer, select the
   following packages:

  ``git, curl, libsasl2, ca-certificates``

2. Start a Cygwin Terminal.
3. Configure the terminal to ignore Windows-style line endings in scripts:

  .. code-block:: sh

    $ echo "set -o igncr && export SHELLOPTS" >> ~/.bashrc && source ~/.bashrc

4. Continue to the :ref:`all platforms <all-platforms>` section.

OS X
--------

If you already have Git installed, you can skip ahead to the :ref:`all platforms
<all-platforms>` section

1. Open the Terminal app.
2. Install `Homebrew <http://mxcl.github.com/homebrew/>`_:
   ``ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"``
3. Install Git with Homebrew (``brew install git``).
4. Continue to the :ref:`all platforms <all-platforms>` section.

Linux
-----

1. Install Git from your distribution's package manager.

   Ubuntu:

  .. code-block:: sh

    $ sudo apt-get install git

  Arch Linux:

  .. code-block:: sh

    $ [sudo] pacman -S git

2. Continue to the :ref:`all platforms <all-platforms>` section.

.. _all-platforms:

All Platforms
-------------

1. If your network uses an Internet proxy (e.g. a corporate network) set the
   ``http_proxy`` and ``https_proxy`` environment variables:

  .. code-block:: sh

    $ export http_proxy=<your proxy>
    $ export https_proxy=<your proxy>

2. Clone the `cantranslator <https://github.com/openxc/cantranslator>`_
   repository:

  .. code-block:: sh

    $ git clone https://github.com/openxc/cantranslator

Flashing
========

USB Cable
---------

You need to have the **mini-USB** port on the chipKIT connected to your computer
to upload a new firmware. This is different than the micro-USB port that you use
to read vehicle data - see the `device connections
<http://openxcplatform.com/vehicle-interface/index.html#connections>`_ section
of the `OpenXC website`_ to make sure you have the correct cable attached.

Uploading Script
------------

Open a terminal and ``cd`` into the ``cantranslator`` folder that you cloned
with Git. Run the ``upload_hex.sh`` script with the ``.hex`` file you
downloaded:

.. code-block:: sh

   $ script/upload_hex.sh <firmware file you downloaded>.hex

The ``upload_hex.sh`` script attempts to install all required dependencies
automatically, and it is tested in Cygwin, OS X Mountain Lion, Ubuntu 12.04 and
Arch Linux - other operating systems may need to :ref:`install the dependencies
manually <manual-deps>`.

If you have more than one virtual serial (COM) port active, you may need to
explicitly specify which port to use. Pass the port name as the second argument
to the script, e.g. in Linux:

.. code-block:: sh

   $ script/upload_hex.sh <firmware file you downloaded>.hex /dev/ttyUSB2

and in Windows, e.g. if you needed to use ``com4`` instead of the default
``com3``:

.. code-block:: sh

   $ script/upload_hex.sh <firmware file you downloaded>.hex com4

**Windows notes:**

In Windows, this command will only work in Cygwin, not the standard
``cmd.exe`` or Powershell.

If you get errors about ``$'\r': command not found`` then your Git configuration
added Windows-style ``CRLF`` line endings. Run this first to ignore the ``CR``:

.. code-block:: sh

   $ set -o igncr && export SHELLOPTS

.. _`MPIDE`: https://github.com/chipKIT32/chipKIT32-MAX/downloads
.. _`OpenXC website`: http://openxcplatform.com

.. _manual-deps:

Dependencies
============

If the bootstrap script failed, you will need to install the dependencies manually. You will need:

* ``avrdude``
* FTDI Driver

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
   - Install `Cygwin <http://cygwin.com>`_ and `MPIDE`_, and follow the
     :doc:`installation` documentation to configure the MPIDE environment
     variables.

.. _`Homebrew`: http://mxcl.github.com/homebrew/
