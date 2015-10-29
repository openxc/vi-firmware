============================
Example Build Configurations
============================

There are many :doc:`makefile-opts`, so it may be difficult to tell which you
need to configure to get a working build. This page collects a few examples of
popular configurations, many of which have shortcuts that can be run with
``fab`` at the command line, without remembering all of the configuration
options.

.. contents::
    :local:
    :depth: 1

Default Build
=============

You must always specify your target platform with the ``PLATFORM`` environment
variable. For this example, we'll set ``PLATFORM=FORDBOARD``.

By default, if you just run ``make`` with no other environment variables set,
the firmware is built with these options. This is a good build to use if you
want to send diagnostic requests through a VI:

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
    -      USB       = DEFAULT_LOGGING_OUTPUT
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

.. NOTE::
  There's a shortcut for this default build, using the Fabric tool and an
  included script. This will build the default build for the reference VI
  platform:

  .. code-block:: sh

      fab reference build

  and for the CrossChasm C5 Devices, choose one of:

  .. code-block:: sh

      fab c5bt build
      fab c5cell build

  and finally, for the chipKIT Max32:

  .. code-block:: sh

      fab chipkit build

  Get the idea? These shortcuts will make sure the flags are set to their
  defaults, regardless of what you may have in your current shell environment.

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
    -      USB       = DEFAULT_LOGGING_OUTPUT
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

.. NOTE::
  This build also has a shortcut using the Fabric script. Just add the keyword
  ``translated_obd2`` before ``build`` in your call to ``fab`` at the command line.
  For example, this compiles for the reference VI with the automatic recurring,
  translated OBD2 requests:

  .. code-block:: sh

      fab reference translated_obd2 build

Emulated Data Build
===================

If you want to test connectivity to a VI from your client device without going
to a vehicle, but you don't care about the actual vehicle data being generated,
you can compile a build that generates random vehicle data and sends it via the
normal I/O interfaces.

If you are building an app, you'll want to use a _`trace file
<http://openxcplatform.com/resources/traces.html>` or the _`vehicle simulator
<https://github.com/openxc/openxc-vehicle-simulator>`.

The config a VI to emulate a vehicle:

.. code-block:: sh

    $ export DEFAULT_EMULATED_DATA_STATUS=1
    $ export DEFAULT_POWER_MANAGEMENT=ALWAYS_ON
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
    -      USB       = DEFAULT_LOGGING_OUTPUT
    -      JSON      = DEFAULT_OUTPUT_FORMAT
    -      0         = DEFAULT_EMULATED_DATA_STATUS
    -      OBD2_IGNIT= DEFAULT_POWER_MANAGEMENT
    -      0x1       = DEFAULT_USB_PRODUCT_ID
    -      0         = DEFAULT_CAN_ACK_STATUS
    -      1         = DEFAULT_OBD2_BUS
    -      1         = DEFAULT_RECURRING_OBD2_REQUESTS_STATUS

There are 2 changes from the default build:

* ``DEFAULT_EMULATED_DATA_STATUS`` is ``1``, which will cause fake data to be
  generated and published from the VI.
* ``DEFAULT_POWER_MANAGEMENT`` is ``ALWAYS_ON``, so the VI will not go to sleep
  while plugged in. Make sure to clear this configuration option before making a
  build to run in a vehicle, or you could drain the battery!

.. NOTE::
  This build also has a shortcut using the Fabric script. Just add the keyword
  ``emulator`` before ``build`` in your call to ``fab`` at the command line.
  For example, this compiles for the reference VI with emulatded data:

  .. code-block:: sh

      fab reference emulator build

Fabric Shortcuts
================

The repository includes a ``fabfile.py`` script, which works with the ``Fabric``
commmand line utility to simplify some of these build configurations. The
``fab`` commands are composable, following this simple formula:

* Start your command with ``fab``
* Specify the target platform with ``chipkit``, ``c5bt``, ``c5cell``, or ``reference``.
* Optionally include ``emulator`` or ``translated_obd2`` to enable one of the
  example builds described above.
* End with ``build`` to start the compilation.

For example, this builds the firmware for a chipKIT and includes emulated data:

.. code-block:: sh

  fab chipkit emulator build

while this builds the default firmware, ready for OBD2 requests for the chipKIT:

.. code-block:: sh

  fab chipkit build

The ``fab`` commands can be run from any folder in the vi-firmware repository.
