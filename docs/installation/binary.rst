==============================
Flashing a Pre-compiled Binary
==============================

Updates to the CAN translator firmware may be distributed as
pre-compiled binaries, e.g. if they are distributed by an OEM who does
not wish to make the CAN signals public. A binary firmware may be distributed
either as a ``.hex`` or ``.bin`` file.

For the moment, all of the pre-compiled firmare are built to run with a
:doc:`bootloader <bootloaders>` on the microcontroller.

Quick Start
============

Windows
-------

1. Install `Cygwin <http://www.cygwin.com>`_ and in the installer, select the
   following packages:

  ``git, curl, libsasl2, ca-certificates, patchutils``

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

3. Continue on to :doc:`platform specific documentation </platforms/platforms>`.

.. _`Homebrew`: http://mxcl.github.com/homebrew/
