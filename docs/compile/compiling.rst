==========
Compiling
==========

For a detailed walkthrough, see :doc:`/getting-started/getting-started`.

The VI firmware can be compiled natively in Linux and Mac OS X. In Windows, you
can use the included virtual machine configuration (which uses
[Vagrant](http://www.vagrantup.com/). Cygwin is unnoficially supported, and is
no longer recommended.

When compiling you need to specify which :doc:`embedded VI platform
</platforms/platforms>` you are compiling for with the ``PLATFORM`` flag. All
other flags are optional.

.. toctree::
    :maxdepth: 2

    example-builds
    makefile-opts
    troubleshooting
    dependencies
    native-development
