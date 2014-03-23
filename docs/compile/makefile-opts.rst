================
Makefile Options
================

These options are passed as shell environment variables to the Makefile, e.g.

.. code-block:: sh

   $ PLATFORM=FORDBOARD make

.. note::

   Try adding the ``-j4`` flag to your calls to ``make`` to build 4 jobs in
   parallel - the speedup can be quite dramatic.

- ``PLATFORM`` - Select the target :doc:`microcontroller platform
  </platforms/platforms>` (see the platform specific pages for valid options).

- ``DEBUG`` - Set to ``1`` to compile with debugging symbols and to enable debug
  logging. By default the logging will be available via the logging USB
  endpoint - for UART output, see the ``UART_LOGGING`` flag. This also implies
  ``DEFAULT_POWER_MANAGEMNET=ALWAYS_ON`` and ``DEFAULT_CAN_ACK_STATUS=1``.

- ``BOOTLOADER`` - By default, the firmware is built to run on a microcontroller
  with a bootloader (if one is available for the selected platform), allowing
  you to update the firmware without specialized hardware. If you want to build
  to run on bare-metal hardware (i.e. start at the top of flash memory) set this
  to ``0``.

- ``TRANSMITTER`` - Set this to ``1`` to imply
  ``DEFAULT_POWER_MANAGEMENT=ALWAYS_ON`` and ``DEFAULT_USB_PRODUCT_ID=0x2``.
  This is useful if you are using the VI as a transmitter in a local CAN bus for
  bench testing. You can address it separately from a receiving VI because of
  the different USB product ID.

- ``DEFAULT_UART_LOGGING_STATUS`` - When combined with ``DEBUG``, set to ``1``
  to enable debug logging via UART. See the :doc:`platform docs
  </platforms/platforms>` for details on how to read this output.

- ``DEFAULT_METRICS_STATUS`` - Set to ``1`` to enable logging CAN message and
  output message statistics over the normal DEBUG output.

- ``DEFAULT_CAN_ACK_STATUS`` - Set to ``1`` to enable write mode on the CAN
  controllers by default, so messages are ACKed. See the :doc:`testing section </testing>`
  for more details.

- ``DEFAULT_ALLOW_RAW_WRITE_NETWORK`` - By default, raw CAN message write
  requests are not allowed from the network interface even if the CAN bus is
  configured to allow raw writes - set this to ``1`` to accept them.

- ``DEFAULT_ALLOW_RAW_WRITE_UART`` - By default, raw CAN message write requests
  are not allowed from the Bluetooth interface even if the CAN bus is configured
  to allow raw writes - set this to ``1`` to accept them.

- ``DEFAULT_ALLOW_RAW_WRITE_USB`` - By default, raw CAN message write requests
  *are* allowed from the wired USB interface (if the CAN bus is also configured
  to allow raw writes) - set this to ``0`` to block them.

- ``DEFAULT_OUTPUT_FORMAT`` - By default, the output format is ``JSON``. Set
  this to ``PROTOBUF`` to use a binary output format, described more in
  :doc:`/advanced/binary`.

- ``DEFAULT_RECURRING_OBD2_REQUESTS_STATUS`` - Set this to ``1`` to include a
  set of recurring OBD-II requests in the build, to be requests immediately on
  startup.

- ``DEFAULT_POWER_MANAGEMENT`` - Valid options are ``ALWAYS_ON``, ``SILENT_CAN``
  and ``OBD2_IGNITION_CHECK``.

- ``DEFAULT_USB_PRODUCT_ID`` - Change the default USB product ID for the device.
  This is useful if you want to address 2 VIs connected to the same computer.

- ``DEFAULT_EMULATED_DATA_STATUS`` - Set this to ``1`` to have the VI generate
  random data and publish it as OpenXC vehicle messages.

- ``DEFAULT_OBD2_BUS`` - Sets the default CAN controller to use for sending
  OBD-II requests. Valid options are ``0`` (don't send any OBD-II requests),
  ``1`` or ``2``. The default value is ``1``.

- ``NETWORK`` - By default, TCP output of OpenXC vehicle data is disabled. Set
  this to ``1`` to enable TCP output on boards that have an Network interface
  (only the chipKIT Max32 right now). (Note that the NETWORK option is broken on
  the chipKIT Max32 build for the moment, see https://github.com/openxc/vi-firmware/issues/189.
