=======================
CAN Message Definition
=======================

Once the libraries are installed and you run ``make``, you'll notice
that it won't compile - you'll get a bunch of errors that look like
this:

.. code-block:: sh

    build-cli/canutil_chipkit.o: In function `initializeCan(CanBus*)':
    canutil_chipkit.cpp:(.text._Z13initializeCanP6CanBus+0xb8): undefined reference to `initializeFilterMasks(unsigned long long, int*)'
    canutil_chipkit.cpp:(.text._Z13initializeCanP6CanBus+0xcc): undefined reference to `initializeFilters(unsigned long long, int*)'
    build-cli/cantranslator.o: In function `receiveWriteRequest(char*)':
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x40): undefined reference to `getSignals()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x48): undefined reference to `getSignalCount()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x7c): undefined reference to `getCommands()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x84): undefined reference to `getCommandCount()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0xa4): undefined reference to `getSignals()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0xac): undefined reference to `getSignalCount()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x118): undefined reference to `getSignals()'
    cantranslator.cpp:(.text._Z19receiveWriteRequestPc+0x120): undefined reference to `getSignalCount()'
    build-cli/cantranslator.o: In function `initializeAllCan()':
    cantranslator.cpp:(.text._Z16initializeAllCanv+0x1c): undefined reference to `getCanBuses()'
    cantranslator.cpp:(.text._Z16initializeAllCanv+0x30): undefined reference to `getCanBusCount()'
    build-cli/cantranslator.o: In function `_ZL17customUSBCallback9USB_EVENTPvj.clone.0':
    cantranslator.cpp:(.text._ZL17customUSBCallback9USB_EVENTPvj.clone.0+0x70): undefined reference to `getMessageSet()'
    cantranslator.cpp:(.text._ZL17customUSBCallback9USB_EVENTPvj.clone.0+0x98): undefined reference to `getMessageSet()'
    build-cli/cantranslator.o: In function `receiveCan(CanBus*)':
    cantranslator.cpp:(.text._Z10receiveCanP6CanBus+0x40): undefined reference to `decodeCanMessage(int, unsigned char*)'
    build-cli/cantranslator.o: In function `loop':
    cantranslator.cpp:(.text.loop+0x1c): undefined reference to `getCanBuses()'
    cantranslator.cpp:(.text.loop+0x30): undefined reference to `getCanBusCount()'
    collect2: ld returned 1 exit status
    make[1]: *** [build-cli/cantranslator.elf] Error 1
    make[1]: Leaving directory `/home/cantranslator/cantranslator'
    make: *** [all] Error 2

The open source repository does not include the implementation of the functions
declared in ``signals.h`` and these are required to compile and program a CAN
transaltor. These functions are dependent on the specific vehicle and message
set, which is often proprietary information to the automaker.

You have three options to get a working CAN translator:

* Implement the functions manually if you know the CAN message formats
* Create a :doc:`CAN message mapping <mappings>` and use the scripts to
  auto-generate signals.cpp. Knowledge of the vehicle's CAN message is also
  required for this method.
* Use a :doc:`pre-built binary firmware </installation/binary>` from an automaker.

Manual
======

You must implement the functions defined in the ``signals.h`` header
file. The documentation of those functions describes the expected effect
of each. Implement these in a file called ``signals.cpp`` and the code
should now compile.

You must know the CAN message formats of the vehicle you want to use
with the CAN translator, as you cannot implement these functions without
that knowledge.

Auto-generated from Mapping
===========================

The code auto-generation script accepts a special :doc:`JSON input file
<mappings>` that defines the CAN messages and signals of interest and rewrites
it as C data structures, ready to be downloaded to the device. You must know the
CAN message formats of the vehicle you want to use with the CAN translator, as
you cannot create these input files without that knowledge.

Once you have one or more input JSON files, run the ``generate_source.py``
script to create a complete implementation of ``signals.cpp`` for your messages.
For example, if your mappings are in ``signals.json``:

.. code-block:: sh

    $ script/generate_code.py --json signals.json > signals.cpp

If you used the ``xml_to_json.py`` script to convert an XML CAN database to
JSON, make sure to provide both the converted file along with your mappings:

.. code-block:: sh

    $ script/generate_code.py --json signals.json --json mappings.json > signals.cpp

Drop the new ``signals.cpp`` file in the ``src`` folder, and it should now
:doc:`compile </installation/compiling>`. Don't add anything else to this file -
it's derivative of the master JSON, and should be able to be wiped and recreated
at any time.

If you have multiple CAN buses and want to define their signals and
messages in separate files, just pass multiple JSON files:

.. code-block:: sh

    $ script/generate_code.py --json highspeed.json --json mediumspeed.json > signals.cpp

Note that the JSON files are parsed and merged, so if you want to define
custom handlers and states separately from the signal definition itself,
you can store them in separate files and they will be merged on import.
