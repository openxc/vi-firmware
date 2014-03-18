======================
Firmware Configuration
======================

The open source repository does not include the implementation of the functions
declared in ``signals.h`` and these are required to compile and program a
vehicle interface. These functions are dependent on the specific vehicle and
message set, which is often proprietary information to the automaker.

If you cannot use a `pre-built binary firmware
<http://openxcplatform.com/vehicle-interface/firmware.html>`_ from an automaker,
you can either:

* *Recommended*: Create a **VI configuration file** and use the `code generation tool
  <http://python.openxcplatform.com/en/latest/tools/codegen.html>`_ in the
  OpenXC Python library. Many :doc:`examples of configuration files <examples>`
  are included in the docs, as well as a :doc:`complete reference <reference>`
  for all configuration options.
* Implement the functions in ``signals.h`` manually. Knowledge of the vehicle's
  CAN message is required for this method. The documentation of those functions
  describes the expected effect of each. Implement these in a file called
  ``signals.cpp`` and the code should now compile. The configuration file
  method is strongly recommended as it still allows flexibility while removing
  boilerplate.

Knowledge of the vehicle's CAN messages is required for both options - this is
advanced territory, and if you're just looking to get some data out of your car
you most likely want one of the binary firmwares from an automaker

.. toctree::
    :maxdepth: 1

    getting-started
    examples
    raw-examples
    code-examples
    write-examples
    faq
    bit-numbering
    reference

Compile-time Configuration Options
==================================

TODO store these in the VI config JSON but have sane defaults.

Debugging
---------

This can only be configured at compile-time. It enables compiling with debug
symbols, sets the optimization level for the compiler to 0 and changes the log
level to debug.

Output Format
-------------

* JSON
* Protobuf

Power Management
----------------

* Off
* Wait for silent CAN bus
* Infer ignition status via OBD-II (be aware this mode will cause the VI to
  write messages to the bus)

OBD-II Recurring Requests
-------------------------

* On
* Off

Recurring and one-time.

USB Product ID
--------------

Anything you want - this can be used if you want to connect to 2 VIs from the
same computer for bench testing.

Emulated Data
-------------

On or off.

Raw and Diagnostics Write Access
--------------------------------

* Allow raw CAN message writes and diagnostic requests made from USB
* Allow raw CAN message writes and diagnostic requests made from Bluetooth
* Allow raw CAN message writes and diagnostic requests made from the network

Send CAN ACKs
-------------

* True - will be an active CAN bus participant and send low-level ACKs. If the
  bus speed is incorrect, can interfere with normal bus operation. This is
  useful if you are bench testing with 2 VIs and you need the CAN messages to be
  propagated up the stack.
* False - will be a listen only node and will not ACK messages. An incorrect bus
  speed will not have a negative impact on the bus, but you still won't be able
  to read anything.

Log Level
---------

* OFF
* WARN
* INFO
* DEBUG

UART Logging
------------

* On or off.

Statistics Tracking
-------------------

Refactor the "LOG_STATS" option to make it faster, take less memory, and send
the results as a JSON object instead of logging it.

* On or off.

CAN Controller Options
----------------------

For controllers 1 and 2:

* Bus speed - off, 125000 or 500000.

Filtered CAN Messages
---------------------

TODO

Translated CAN signals
----------------------

TODO

Diagnostic Requests
-----------------------------

TODO
