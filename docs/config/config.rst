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
