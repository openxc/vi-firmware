============================
Testing with Emulated Data
============================

At this point, we will assume you've :doc:`set up your development environment
</getting-started/development-environment>` and continue on to test it.

You can confirm your development environment is set up correctly by compiling the
firmware with the default configuration. This build is not configured to read
any particular CAN signals or messages, but it allow you to send On-Board
Diagnostic (OBD-II) requests and raw CAN messages for experimentation.

If you are using Vagrant, ``cd`` into the ``/vagrant`` directory (which is
actually a pointer to the ``vi-firmware`` directory on your host computer) and
run ``fab reference build``:

.. code-block:: sh

    $ cd /vagrant
    /vagrant $ fab reference emulator build
    Compiling for FORDBOARD...
    ...lots of output...
    Compiled successfully for FORDBOARD running under a bootloader.

There will be a lot more output when you run this but it should end with
``Compiled successfully...``. If you got an error, try and follow what
it recommends, then look at the troubleshooting section, and finally ask
for help on the `Google Group </overview/discuss.html>`_.

The compiled firmware is located at
``src/build/FORDBOARD/vi-firmware-FORDBOARD.bin``. Find the instructions for
re-flashing your VI on the :doc:`supported hardware platforms
</platforms/platforms>` page and flash with this new firmware. For other
platforms, the location will be slightly different in the ``build`` directory -
e.g. for the ``CHIPKIT`` platform it will be at
``src/build/CHIPKIT/vi-firmware-CHIPKIT.hex``.

Finally, test that you can receive the emulated data output stream using the
OpenXC Python library:

#. You should already have the OpenXC Python library installed after running the
   ``bootstrap.sh`` script, but if not, `install the library
   <http://python.openxcplatform.com/#installation>`_ with ``pip``. Don't forget
   a `USB backend <http://python.openxcplatform.com/en/latest/#usb>`_.
#. Attach the programmed VI to your computer with a USB cable. In Windows,
   install the `VI windows driver
   <https://github.com/openxc/vi-windows-driver>`_.
#. Run ``openxc-control version`` from the command line - it should print out the
   current firmware version of the attached vehicle interface. If you instead
   get an error about not being able to find the USB device, make sure the VI
   has power (look for an LED).
#. If the version check was successful, run ``openxc-dump`` to view the raw data
   stream of emulated vehicle data coming from the VI.
