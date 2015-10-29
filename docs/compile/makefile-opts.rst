================
Makefile Options
================

These options are passed as shell environment variables to the Makefile, e.g.

.. code-block:: sh

   $ PLATFORM=FORDBOARD make

.. note::

   Try adding the ``-j4`` flag to your calls to ``make`` to build 4 jobs in
   parallel - the speedup can be quite dramatic.

.. note::

   Don't miss the ``Fabric`` helper scripts for the :doc:`most common build
   configurations <example-builds>`.

``PLATFORM``
  Select the target :doc:`microcontroller platform </platforms/platforms>`.

  Values: ``FORDBOARD, CHIPKIT, CROSSCHASM_C5_BT, CROSSCHASM_C5_CELLULAR, BLUEBOARD``

  Default: ``CHIPKIT``

.. note::
   
   The old Platform variable ``CROSSCHASM_C5`` has been renamed to ``CROSSCHASM_C5_BT``

``DEBUG``
  Set to ``1`` to compile with debugging symbols and to enable debug logging. By
  default the logging will be available via the logging USB endpoint - for UART
  output, see the ``DEFAULT_LOGGING_OUTPUT`` flag. This also forces
  ``DEFAULT_POWER_MANAGEMENT=ALWAYS_ON`` and ``DEFAULT_CAN_ACK_STATUS=1``.

  Values: ``0`` or ``1``

  Default: ``0``

``BOOTLOADER``
  By default, the firmware is built to run on a microcontroller with a
  bootloader (if one is available for the selected platform), allowing you to
  update the firmware without specialized hardware. If you want to build to run
  on bare-metal hardware (i.e. start at the top of flash memory) set this to
  ``0``.

  Values: ``0`` or ``1``

  Default: ``1``

``TRANSMITTER``
  Set this to ``1`` to force
  ``DEFAULT_POWER_MANAGEMENT=ALWAYS_ON`` and ``DEFAULT_USB_PRODUCT_ID=0x2``.
  This is useful if you are using the VI as a transmitter in a local CAN bus for
  bench testing. You can address it separately from a receiving VI because of
  the different USB product ID.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_LOGGING_OUTPUT``
  When combined with ``DEBUG``, controls the output interface used for debug logging.
  See the :doc:`platform docs </platforms/platforms>` for details on how to read
  this output.

  Values: ``OFF``, ``USB``, ``UART`` or ``BOTH``

  Default: ``USB``

``DEFAULT_METRICS_STATUS``
  Set to ``1`` to enable logging CAN message and output message statistics over
  the normal DEBUG output.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_CAN_ACK_STATUS``
  If 1, the VI will be an active CAN bus participant and send low-level ACKs. If
  the bus speed is incorrect, can interfere with normal bus operation. This is
  useful if you are bench testing with 2 VIs and you need the CAN messages to be
  propagated up the stack.

  If 0, the VI will be a listen only node and will not ACK messages. An
  incorrect bus speed will not have a negative impact on the bus, but you still
  won't be able to read anything.

  See the :doc:`testing section </testing>` for more details.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_ALLOW_RAW_WRITE_NETWORK``
  By default, raw CAN message write requests are not allowed from the network
  interface even if the CAN bus is configured to allow raw writes - set this to
  ``1`` to accept them.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_ALLOW_RAW_WRITE_UART``
  By default, raw CAN message write requests are not allowed from the Bluetooth
  interface even if the CAN bus is configured to allow raw writes - set this to
  ``1`` to accept them.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_ALLOW_RAW_WRITE_USB``
  By default, raw CAN message write requests *are* allowed from the wired USB
  interface (if the CAN bus is also configured to allow raw writes) - set this
  to ``0`` to block them.

  Values: ``0`` or ``1``

  Default: ``1``

``DEFAULT_OUTPUT_FORMAT``
  By default, the output format is ``JSON``. Set this to ``PROTOBUF`` to use a
  binary output format, described more in :doc:`/advanced/binary`.

  Values: ``JSON``, ``PROTOBUF``

  Default: ``JSON``

``DEFAULT_RECURRING_OBD2_REQUESTS_STATUS``
  Set this to ``1`` to include a set of recurring OBD-II requests in the build,
  to be requests immediately on startup.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_POWER_MANAGEMENT``
  Valid options are ``ALWAYS_ON``, ``SILENT_CAN`` and ``OBD2_IGNITION_CHECK``.

  Values: ``ALWAYS_ON``, ``SILENT_CAN``, ``OBD2_IGNITION_CHECK`` (will cause the
  VI to write messages to the bus)

  Default: ``SILENT_CAN``

``DEFAULT_USB_PRODUCT_ID``
  Change the default USB product ID for the device. This is useful if you want
  to address 2 VIs connected to the same computer.

  Values: ``0x0`` to ``0xffff``

  Default: ``0x1``

``DEFAULT_EMULATED_DATA_STATUS``
  Set this to ``1`` to have the VI generate random data and publish it as OpenXC
  vehicle messages.

  Values: ``0`` or ``1``

  Default: ``0``

``DEFAULT_OBD2_BUS``
  Sets the default CAN controller to use for sending OBD-II requests. Valid
  options are ``0`` (don't send any OBD-II requests), ``1`` or ``2``. The
  default value is ``1``.

  Values: ``0`` (off), ``1`` or ``2``

  Default: ``1``

``NETWORK``
  By default, TCP output of OpenXC vehicle data is disabled. Set this to ``1``
  to enable TCP output on boards that have an Network interface. Note that the
  NETWORK option is broken on the chipKIT Max32 build for the moment, see
  https://github.com/openxc/vi-firmware/issues/189.

  Values: ``0`` or ``1``

  Default: ``0``
