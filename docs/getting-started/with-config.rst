=====================================
Compiling with a Custom Configuration
=====================================

The VI firmware doesn't understand the JSON configuration file format natively,
so next you have to convert it to a C++ implementation. The OpenXC Python
library includes a tool that will *generate the C++* for ``signals.cpp`` from
your configuration file, so you still don't have to write any code.

You should already have the Python library installed from the environment setup
you did earlier (and if not, run `pip install openxc`). Assuming the
``accelerator-config.json`` file we created is in our home directory, run this
to generate a valid ``signals.cpp`` for our CAN signal:

.. code-block:: sh

   vi-firmware/src $ openxc-generate-firmware-code --message-set ~/accelerator-config.json > signals.cpp

and then re-compile the firmware:

.. code-block:: sh

    vi-firmware $ fab reference build
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
signals simultaneously, raw CAN messages, diagnostic reqwuests, advanced data
transformation, etc. For complete details, see the `VI Firmware docs
<http://vi-firmware.openxcplatform.com/>`_. You can find the right ``PLATFORM``
value for your VI in the `VI firmware supported platforms page
<http://vi-firmware.openxcplatform.com/en/latest/platforms/platforms.html>`_.

