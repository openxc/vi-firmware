=======================
CAN Message Definition
=======================

The open source repository does not include the implementation of the functions
declared in ``signals.h`` and these are required to compile and program a CAN
transaltor. These functions are dependent on the specific vehicle and message
set, which is often proprietary information to the automaker.

Once the libraries are installed and you run ``make``, you'll notice that it
won't compile - you'll get a bunch of errors about undefined references to
functions from ``signals.h``:

.. code-block:: sh

  build/pic32/cantranslator.o: In function `updateDataLights()':
  cantranslator.cpp:(.text._Z16updateDataLightsv+0x20): undefined reference to `openxc::signals::getCanBusCount()'
  cantranslator.cpp:(.text._Z16updateDataLightsv+0x48): undefined reference to `openxc::signals::getCanBusCount()'
  cantranslator.cpp:(.text._Z16updateDataLightsv+0xd4): undefined reference to `openxc::signals::getCanBuses()'
  build/pic32/cantranslator.o: In function `initializeAllCan()':
  cantranslator.cpp:(.text._Z16initializeAllCanv+0x1c): undefined reference to `openxc::signals::getCanBuses()'
  cantranslator.cpp:(.text._Z16initializeAllCanv+0x30): undefined reference to `openxc::signals::getCanBusCount()'
  build/pic32/cantranslator.o: In function `setup':
  cantranslator.cpp:(.text.setup+0x14): undefined reference to `openxc::signals::initialize()'
  build/pic32/cantranslator.o: In function `receiveRawWriteRequest(cJSON*, cJSON*)':
  cantranslator.cpp:(.text._Z22receiveRawWriteRequestP5cJSONS0_+0x3c): undefined reference to `openxc::signals::getCanBuses()'
  build/pic32/cantranslator.o: In function `receiveTranslatedWriteRequest(cJSON*, cJSON*)':
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x44): undefined reference to `openxc::signals::getSignals()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x4c): undefined reference to `openxc::signals::getSignalCount()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x78): undefined reference to `openxc::signals::getSignals()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x80): undefined reference to `openxc::signals::getSignalCount()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0xe4): undefined reference to `openxc::signals::getCommands()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0xec): undefined reference to `openxc::signals::getCommandCount()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x10c): undefined reference to `openxc::signals::getSignals()'
  cantranslator.cpp:(.text._Z29receiveTranslatedWriteRequestP5cJSONS0_+0x114): undefined reference to `openxc::signals::getSignalCount()'
  build/pic32/cantranslator.o: In function `receiveCan(openxc::pipeline::Pipeline*, CanBus*)':
  cantranslator.cpp:(.text._Z10receiveCanPN6openxc8pipeline8PipelineEP6CanBus+0x54): undefined reference to `openxc::signals::decodeCanMessage(openxc::pipeline::Pipeline*, CanBus*, int, unsigned long long)'
  build/pic32/cantranslator.o: In function `loop':
  cantranslator.cpp:(.text.loop+0x2c): undefined reference to `openxc::signals::getCanBuses()'
  cantranslator.cpp:(.text.loop+0x44): undefined reference to `openxc::signals::getCanBusCount()'
  cantranslator.cpp:(.text.loop+0x90): undefined reference to `openxc::signals::getCanBuses()'
  cantranslator.cpp:(.text.loop+0xa4): undefined reference to `openxc::signals::getCanBusCount()'
  cantranslator.cpp:(.text.loop+0xd4): undefined reference to `openxc::signals::loop()'
  build/pic32/main.o: In function `main':
  main.cpp:(.text.main+0x60): undefined reference to `openxc::signals::getActiveMessageSet()'
  build/pic32/main.o: In function `handleControlRequest(unsigned char)':
  main.cpp:(.text._Z20handleControlRequesth+0x64): undefined reference to `openxc::signals::getActiveMessageSet()'
  main.cpp:(.text._Z20handleControlRequesth+0x8c): undefined reference to `openxc::signals::getActiveMessageSet()'
  build/pic32/platform/pic32/canutil.o: In function `openxc::can::initialize(CanBus*)':
  canutil.cpp:(.text._ZN6openxc3can10initializeEP6CanBus+0xbc): undefined reference to `openxc::signals::initializeFilters(unsigned long long, int*)'
  build/pic32/platform/platform.o: In function `openxc::platform::suspend(openxc::pipeline::Pipeline*)':
  platform.cpp:(.text._ZN6openxc8platform7suspendEPNS_8pipeline8PipelineE+0x3c): undefined reference to `openxc::signals::getCanBuses()'
  platform.cpp:(.text._ZN6openxc8platform7suspendEPNS_8pipeline8PipelineE+0x50): undefined reference to `openxc::signals::getCanBusCount()'
  collect2: ld returned 1 exit status
  make: *** [build/pic32/cantranslator-pic32.elf] Error

You have three options to get a working vehicle interface:

* Use a :doc:`pre-built binary firmware </installation/binary>` from an automaker
* Create a message set mapping and use the `OpenXC Python library
  <http://python.openxcplatform.com>`_ to auto-generate an implementation of
  ``signals.h``. Knowledge of the vehicle's CAN message is required for this
  method.
* Implement the ``signals.h`` functions manually

Auto-generated from Mapping
===========================

The code generation tools are documented in the `code generation input
definitions <http://python.openxcplatform.com/code-generation.html>`_.

Once you've defined your message set in a JSON file, install the `OpenXC Python
library <http://python.openxcplatform.com>`_, then run the
`openxc-generate-firmware-code` tool to create an implementation of
``signals.cpp``:

.. code-block:: sh

    cantranslator/ $ openxc-generate-firmware-code --message-set mycar.json > src/signals.cpp

The firmware should now :doc:`compile </installation/compiling>`! Don't modify
the ``signals.cpp`` file manually, since it's generated you should expect it to
be wiped and recreated at any time; always make changes to the JSON instead.

Manual Implementation
=====================

You must implement the functions defined in the ``signals.h`` header
file. The documentation of those functions describes the expected effect
of each. Implement these in a file called ``signals.cpp`` and the code
should now compile.

You must know the CAN message formats of the vehicle you want to use with the
vehicle interface, as you cannot implement these functions without that
knowledge.
