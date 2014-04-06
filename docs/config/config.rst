======================
Firmware Configuration
======================

If you cannot use a `pre-built binary firmware
<http://openxcplatform.com/vehicle-interface/firmware.html>`_ from an automaker
you can create a **VI configuration file** and use the `code generation tool
<http://python.openxcplatform.com/en/latest/tools/codegen.html>`_ in the OpenXC
Python library. Many :doc:`examples of configuration files <examples>` are
included in the docs, as well as a :doc:`complete reference <reference>` for all
configuration options

Knowledge of the car's CAN messages is required to build a custom configuration
file. If you're just looking to get some data out of your car you most likely
want a binary firmware from your car's maker. If they don't offer one, get
together with the community to reverse engineer it!

.. toctree::
    :maxdepth: 1

    examples
    raw-examples
    code-examples
    write-examples
    raw-write-examples
    diagnostic
    bit-numbering
    reference
    faq
