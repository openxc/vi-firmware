====================
Building from Source
====================

For a detailed walkthrough, see :doc:`getting-started`.

The build process uses GNU Make and works with Linux (tested in Arch Linux and
Ubuntu), OS X and Cygwin 32-bit in Windows. For documentation on how to build
for each platform, see the :doc:`supported platform details
</platforms/platforms>`.

Before you can compile, you will need to define your CAN messages :doc:`define
your CAN messages </config/config>`. When compiling you need to specify which
board you are compiling for with the ``PLATFORM`` flag. All other flags are
optional.

.. toctree::
    :maxdepth: 2

    getting-started
    makefile-opts
    troubleshooting
