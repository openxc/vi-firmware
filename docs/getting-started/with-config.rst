=====================================
Compiling with a Custom Configuration
=====================================

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

    vi-firmware/src $ make clean
    vi-firmware/src $ PLATFORM=FORDBOARD make
    Compiling for FORDBOARD...
    ...
    15 Compiling build/FORDBOARD/signals.o
    Producing build/FORDBOARD/vi-firmware-FORDBOARD.elf
    Producing build/FORDBOARD/vi-firmware-FORDBOARD.bin
    Compiled successfully for FORDBOARD running under a bootloader.

Success! The compiled firmware is located at
``build/FORDBOARD/vi-firmware-FORDBOARD.bin``. We can use the same VI
re-flashing procedure that we used for a binary firmware from an
automaker with our custom firmware - the process is going to depend on
the VI you have, so see the list of `supported
VIs </vehicle-interface/hardware.html>`_ to find the right
instructions.

There's a *lot* more you can do with the firmware - many more CAN
signals simultaneously, raw CAN messages, advanced data transformation,
etc. For complete details, see the `VI Firmware
docs <http://vi-firmware.openxcplatform.com/>`_. You can find the right
``PLATFORM`` value for your VI in the `VI firmware supported platforms
page <http://vi-firmware.openxcplatform.com/en/latest/platforms/platforms.html>`_.

