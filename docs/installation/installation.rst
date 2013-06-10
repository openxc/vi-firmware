============
Installation
============

If you've downloaded a pre-built binary for a specific vehicle, see the
:doc:`binary` section for instructions on how to flash your CAN
translator. Most users do not need to set up the full development described in
these docs.

.. toctree::
  :maxdepth: 1

  binary
  compiling
  testing
  bootloaders
  dependencies

Quick Start
============

Linux
-----

1. Install Git from your distribution's package manager.

   Ubuntu:

  .. code-block:: sh

    $ sudo apt-get install git

  Arch Linux:

  .. code-block:: sh

    $ [sudo] pacman -S git

2. Continue to the :ref:`all platforms <all-plats>` section.

Windows
-------

1. Install `Cygwin <http://www.cygwin.com>`_ and in the installer, select the
   following packages:

  ``gcc4, patchutils, git, unzip, python, python-argparse, check, curl,
  libsasl2, ca-certificates, python-setuptools``

2. Start a Cygwin Terminal.
3. Configure the terminal to ignore Windows-style line endings in scripts:

  .. code-block:: sh

    $ set -o igncr && export SHELLOPTS

4. Install the :ref:`FTDI driver <ftdi>` (the bootstrap script tries to take
   care of this, but some developers are reporting that it doesn't actaully get
   installed)
5. Continue to the :ref:`all platforms <all-plats>` section.

OS X
--------

1. Open the Terminal app.
2. Install `Homebrew <http://mxcl.github.com/homebrew/>`_:
   ``ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"``
3. Install Git with Homebrew (``brew install git``).
4. Continue to the :ref:`all platforms <all-plats>` section.

.. _all-plats:

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

3. Run the ``bootstrap.sh`` script:

  .. code-block:: sh

    $ cd cantranslator
    $ script/bootstrap.sh

4. If there were no errors, you are ready to :doc:`compile <compiling>`. If
   there are errors, follow the recommendations in the error messages. You may
   need to manually install the dependencies if your environment is not in a
   predictable state.

The ``bootstrap.sh`` script is tested in Cygwin, OS X Mountain Lion, Ubuntu
12.04 and Arch Linux - other operating systems may need to
:doc:`install the dependencies <dependencies>` manually.

