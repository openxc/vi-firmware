=====================================
Getting Started with VI Compilation
=====================================

At this point, we'll assume you already have a VI configuration file - either
one you wrote yourself, or the configuration from the :doc:`tutorial
</config/getting-started>`.

Environment Setup
-----------------

Before we can compile, we need to set up our development environment.
Bear with it...there's a few steps but you only have to do it once!

When you see ``$`` it means this is a shell command - run the command
after the ``$`` but don't include it. The example shell commands may
also be prefixed with a folder name, if it needs to be run from a
particular location.

Windows
^^^^^^^

Download `Cygwin <http://www.cygwin.com>`__ and run the installer -
during the installation process, select these packages:

::

    make, gcc-core, patchutils, git, unzip, python, check, curl, libsasl2, python-setuptools

After it's installed, open a new Cygwin terminal and configure it to
ignore Windows-style line endings in scripts by running this command:

.. code-block:: sh

   $ set -o igncr && export SHELLOPTS

Linux
^^^^^

We need to install Git from our distribution's package manager.

In Ubuntu:

.. code-block:: sh

   $ sudo apt-get install git

In Arch Linux:

.. code-block:: sh

   $ [sudo] pacman -S git

OS X
^^^^

Open the Terminal app and install
`Homebrew <http://mxcl.github.com/homebrew/>`__ by running this command:

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

Clone the `vi-firmware <https://github.com/openxc/vi-firmware>`__
repository:

.. code-block:: sh

   $ git clone https://github.com/openxc/vi-firmware

Run the ``bootstrap.sh`` script:

.. code-block:: sh

   $ cd vi-firmware
   vi-firmware/ $ script/bootstrap.sh

If there were no errors, we are ready to compile. If there are errors,
try to follow the recommendations in the error messages. You may need to
:doc:`manually install the dependencies </dependencies>` if your environment is not in a
predictable state. The ``bootstrap.sh`` script is tested in Cygwin, OS X
Mountain Lion, Ubuntu 12.04 and Arch Linux.

Testing Compilation
-------------------

Let's confirm the development environment is set up correctly by
compiling the emulator version of the firmware. Move to the
``vi-firmware/src`` directory and compile the emulator for the
`reference VI from Ford <http://vi.openxcplatform.com>`__:

.. code-block:: sh

    vi-firmware/ $ cd src
    vi-firmware/src $ PLATFORM=FORDBOARD make emulator
    Compiling for FORDBOARD...
    ...
    Compiled successfully for FORDBOARD running under a bootloader.

There will be a lot more output when you run this but it should end with
``Compiled successfully...``. If you got an error, try and follow what
it recommends, then look at the troubleshooting section, and finally ask
for help on the `Google Group </overview/discuss.html>`__.

Compiling, False Start
----------------------

Assuming the emulator compiled successfully , we're ready to build for
an actual live CAN bus. Clean up the build to make sure the emulator
doesn't conflict:

.. code-block:: sh

   vi-firmware/src $ make clean

and then compile for a real vehicle - just leave off the ``emulator``
option:

.. code-block:: sh

   vi-firmware/src $ PLATFORM=FORDBOARD make

Whoa - we just a bunch of errors about an ``undefined reference`` like
this:

::

    vi_firmware.cpp:55: undefined reference to `openxc::signals::getCanBuses()'

What's going on? The open source VI firmware doesn't include any CAN
message definitions by default. In fact there is no implementation of
the C++ file ``signals.cpp``, one that's required to build - we need to
implement the functions defined in ``signals.h`` before the firmware
will compile.

Compiling with the Missing Link
-------------------------------

Didn't we say that we didn't have to know how to write C++, though?
Correct! We've reached the final piece - the OpenXC Python library
includes a tool that will *generate the C++* for ``signals.cpp`` from
the JSON configuration file we wrote earlier.

From the environment setup, we already have the OpenXC Python library
installed. Assuming the ``accelerator-config.json`` file is in our home
directory, run this to generate a valid ``signals.cpp`` for our CAN
signal:

.. code-block:: sh

   vi-firmware/src $ openxc-generate-firmware-code --message-set ~/accelerator-config.json > signals.cpp

and then try compiling again:

.. code-block:: sh

    vi-firmware/src $ PLATFORM=FORDBOARD make
    Compiling for FORDBOARD...
    15 Compiling build/FORDBOARD/signals.o
    Producing build/FORDBOARD/vi-firmware-FORDBOARD.elf
    Producing build/FORDBOARD/vi-firmware-FORDBOARD.bin
    Compiled successfully for FORDBOARD running under a bootloader.

Success! The compiled firmware is located at
``build/FORDBOARD/vi-firmware-FORDBOARD.bin``. We can use the same VI
re-flashing procedure that we used for a binary firmware from an
automaker with our custom firmware - the process is going to depend on
the VI you have, so see the list of `supported
VIs </vehicle-interface/hardware.html>`__ to find the right
instructions.

There's a *lot* more you can do with the firmware - many more CAN
signals simultaneously, raw CAN messages, advanced data transformation,
etc. For complete details, see the `VI Firmware
docs <http://vi-firmware.openxcplatform.com/>`__. YOu can find the right
``PLATFORM`` value for your VI in the `VI firmware supported platforms
page <http://vi-firmware.openxcplatform.com/en/latest/platforms/platforms.html>`__.

.. toctree::
    :maxdepth: 1

    ../dependencies
