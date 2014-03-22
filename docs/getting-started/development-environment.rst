=============================
Development Environment Setup
=============================

Before we can compile, we need to set up our development environment.
Bear with it...there's a few steps but you only have to do it once!

When you see ``$`` it means this is a shell command - run the command
after the ``$`` but don't include the ``$``. The example shell commands may
also be prefixed with a folder name, if it needs to be run from a
particular location, e.g. ``foo/ $ ls`` means to run ``ls`` from the ``foo``
folder.

Windows
^^^^^^^

Download the 32-bit version of `Cygwin <http://www.cygwin.com>`_ and run the
installer - during the installation process, select these packages:

::

    make, gcc-core, patchutils, git, unzip, python, check, curl, libsasl2, python-setuptools

After it's installed, open a new Cygwin terminal and configure it to
ignore Windows-style line endings in scripts by running this command:

.. code-block:: sh

   $ set -o igncr && export SHELLOPTS

Linux
^^^^^

Install Git with your Linux distribution's package manager:

Ubuntu:

.. code-block:: sh

   $ sudo apt-get install git

Arch Linux:

.. code-block:: sh

   $ [sudo] pacman -S git

OS X
^^^^

Open the Terminal app and install
`Homebrew <http://mxcl.github.com/homebrew/>`_ by running this command:

.. code-block:: sh

   $ ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"

Once Homebrew is installed, use it to install Git:

.. code-block:: sh

   $ brew install git

All Platforms
^^^^^^^^^^^^^

If we're on a network that requires an Internet proxy (e.g. at work on a
corporate network) set these environment variables.

.. code-block:: sh

   $ export http_proxy=<your proxy>
   $ export https_proxy=$http_proxy
   $ export all_proxy=$http_proxy

Clone the `vi-firmware <https://github.com/openxc/vi-firmware>`_ repository:

.. code-block:: sh

   $ git clone https://github.com/openxc/vi-firmware

Run the ``bootstrap.sh`` script:

.. code-block:: sh

   $ cd vi-firmware
   vi-firmware/ $ script/bootstrap.sh

If there were no errors, we are ready to compile. If there are errors, try to
follow the recommendations in the error messages. You may need to :doc:`manually
install the dependencies </compile/dependencies>` if your environment is not in a
predictable state. The ``bootstrap.sh`` script is tested in 32-bit Cygwin, OS X
Mountain Lion and Mavericks, Ubuntu 13.04 and Arch Linux.
