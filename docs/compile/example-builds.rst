============================
Example Build Configurations
============================

There are many :doc:`makefile-opts`, so it may be difficult to tell which you
need to configure to get a working build. This page collects a few examples of
popular configurations.

Default Build
=============

By default, if you just run ``make`` with no environment variables set, the
firmware is built with these options:

.. code-block:: sh

    $ make
    <...snip lots of output...>
    Compiled with options:
    -      FORDBOARD = PLATFORM
    -      1         = BOOTLOADER
    -      0         = DEBUG
    -      0         = DEFAULT_METRICS_STATUS
    -      1         = DEFAULT_ALLOW_RAW_WRITE_USB
    -      0         = DEFAULT_ALLOW_RAW_WRITE_UART
    -      0         = DEFAULT_ALLOW_RAW_WRITE_NETWORK
    -      0         = DEFAULT_UART_LOGGING_STATUS
    -      JSON      = DEFAULT_OUTPUT_FORMAT
    -      0         = DEFAULT_EMULATED_DATA_STATUS
    -      SILENT_CAN= DEFAULT_POWER_MANAGEMENT
    -      0x1       = DEFAULT_USB_PRODUCT_ID
    -      0         = DEFAULT_CAN_ACK_STATUS
    -      1         = DEFAULT_OBD2_BUS
    -      0         = DEFAULT_RECURRING_OBD2_REQUESTS_STATUS

The Makefile will always print the configuration used so you can double check.

* This default configuration will run on a _`Ford reference VI
  <http://vi.openxcplatform.com/>` (``PLATFORM`` is ``FORDBOARD``) running the
  pre-loaded bootloader (``BOOTLOADER`` is ``1``).
* Debug mode is off (``DEBUG`` is ``0``) so no log messages will be output via
  USB for maximum performance.
* If the VI configuration :ref:`allows raw CAN writes <raw-write-config>`, they
  will only be permitted if set via USB (``DEFAULT_ALLOW_RAW_WRITE_USB`` is ``1``
  but the ``*_UART`` and ``*_NETWORK`` versions are ``0``.
* The data sent from the VI will be serialized to JSON in the format defined by
  the _`OpenXC message format <https://github.com/openxc/openxc-message-format>`.
* The VI will go into sleep mode only when no CAN bus activity is detected for a
  few seconds (the ``DEFAULT_POWER_MANAGEMENT`` mode is ``SILENT_CAN``).
* The CAN controllers will be initialized as listen only unless the VI
  configuration explicitly states they are writable (``DEFAULT_CAN_ACK_STATUS``
  is ``1``). This means that the VI may not work in a bench testing setup where
  nothing else on the bus is ACKing.

Automatic Recurring OBD-II Requests Build
==========================================

Another common build is one that automatically queries the vehicle to check if
it supports a pre-defined set (see the file ``obd2.cpp``) of interesting OBD-II
parameters, and if so, sets up recurring requests for them. Compile with these
options:

.. code-block:: sh

    $ export DEFAULT_RECURRING_OBD2_REQUESTS_STATUS=1
    $ export DEFAULT_POWER_MANAGEMENT=OBD2_IGNITION_CHECK
    $ make
    <...snip lots of output...>

    Compiled with options:
    -      FORDBOARD = PLATFORM
    -      1         = BOOTLOADER
    -      0         = DEBUG
    -      0         = DEFAULT_METRICS_STATUS
    -      1         = DEFAULT_ALLOW_RAW_WRITE_USB
    -      0         = DEFAULT_ALLOW_RAW_WRITE_UART
    -      0         = DEFAULT_ALLOW_RAW_WRITE_NETWORK
    -      0         = DEFAULT_UART_LOGGING_STATUS
    -      JSON      = DEFAULT_OUTPUT_FORMAT
    -      0         = DEFAULT_EMULATED_DATA_STATUS
    -      OBD2_IGNIT= DEFAULT_POWER_MANAGEMENT
    -      0x1       = DEFAULT_USB_PRODUCT_ID
    -      0         = DEFAULT_CAN_ACK_STATUS
    -      1         = DEFAULT_OBD2_BUS
    -      1         = DEFAULT_RECURRING_OBD2_REQUESTS_STATUS

Notice we changed:

* ``DEFAULT_RECURRING_OBD2_REQUESTS_STATUS`` to ``1``. This enables the
  automatic OBD-II queries.
* ``DEFAULT_POWER_MANAGEMENT`` to ``OBD2_IGNITION_CHECK`` (the Makefile summary
  display truncates this value). This changes the power management mode to
  actively probe the vehicle for the engine and vehicle speed. Some vehicles
  will keep modules alive if anyone is making diagnostic requests (e.g. the VI),
  and we want to avoid that because it could drain the car's battery. This mode
  actively infers if the ignition is on and stops sending diagnostic queries if
  we think the car is off. The combination of an engine and vehicle speed check
  should be compatible with hybrid vehicles.
