===============
Getting Started
===============

If you've downloaded a pre-built binary firmware for your car, locate your VI in
the `list of supported interfaces
<http://openxcplatform.com/vehicle-interface/hardware.html>`_ to find
instructions for programming it. You don't need anything from the VI firmware
documentation itself - most users don't need anything in this documentation.
Really, you can stop here!

The first steps for building custom firmware are:

#. Make sure you have one of the :doc:`supported vehicle interfaces </platforms/platforms>`.
#. :doc:`Set up your development environment </getting-started/development-environment>`.
#. :doc:`Compile with emulated data output </getting-started/compiling-emulator>` to make
   sure your development environment is set up, including whatever tool or
   library you plan to use to interact with the VI.
#. :doc:`Compile the default configuration </getting-started/default-build>` so you
   can start reading standard On-Board Diagnostics (OBD-II) data from your car.
#. To go beyond OBD-II, create a :doc:`create a configuration file </getting-started/config>`
   for your car.
#. With a custom configuration in hand, :doc:`compile with your new config </getting-started/with-config>`.

.. toctree::
    :maxdepth: 1

    development-environment
    compiling-emulator
    default-build
    config
    with-config
