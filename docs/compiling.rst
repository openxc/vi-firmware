====================
Building from Source
====================

For a more detailed walkthrough, see the `Getting Started with Custom Data
<http://openxcplatform.com/firmware/custom-data-example.html>`_ guide at the
OpenXC website. The documentation here will assume a higher level of familiarity
with development tools like Git and shell scripts.

The build process uses GNU Make and works with Linux (tested in Arch Linux and
Ubuntu), OS X and Cygwin in Windows. For documentation on how to build for each
platform, see the :doc:`supported platform details </platforms/platforms>`.

Before you can compile, you will need to define your CAN messages :doc:`define
your CAN messages </definitions>`. When compiling you need to specify which
board you are compiling for with the ``PLATFORM`` flag. All other flags are
optional.

Makefile Options
================

These options are passed as shell environment variables to the Makefile, e.g.

.. code-block:: sh

   $ PLATFORM=FORDBOARD make

``PLATFORM`` - Select the target :doc:`microcontroller platform </platforms/platforms>`
   (see the platform specific pages for valid options).

``DEBUG`` - Set to ``1`` to compile with debugging symbols and to enable
      debug logging. See the :doc:`platform docs </platforms/platforms>` for
      details on how to read this output.

``LOG_STATS`` - Set to ``1`` to enable logging CAN message and output message
      statistics over the normal DEBUG output.

``BENCHTEST`` - Set to ``1`` to enable write mode on the CAN controllers so
      messages are ACKed. SEe the :doc:`testing section </testing>`
      for more details.


``NETWORK`` - By default, TCP output of OpenXC vehicle data is disabled. Set
this to ``1`` to enable TCP output on boards that have an Network interface (only
the chipKIT Max32 right now).

``BOOTLOADER`` - By default, the firmware is built to run on a microcontroller
with a bootloader (if one is available for the selected platform), allowing you
to update the firmware without specialized hardware. If you want to build to run
on bare-metal hardware (i.e. start at the top of flash memory) set this to
``0``.

.. note::

   When running ``make`` to compile, try adding the ``-j4`` flag to build jobs
   in parallel - the speedup can be quite dramatic.

Troubleshooting
===============

If the compilation didn't work:

-  Make sure the submodules are up to date - run
   ``git submodule update --init`` and then ``git status`` and make sure
   there are no modified files in the working directory.
-  Did you download the .zip file of the ``vi-firmware`` project from
   GitHub? Use git to clone the repository instead - the library dependencies
   are stored as git submodules and do not work when using the zip file.
-  If you get a lot of errors about ``undefined reference to getSignals()'`` and
   other functions, you need to make sure you defined your CAN messages - read
   through :doc:`/definitions` before trying to compile.
