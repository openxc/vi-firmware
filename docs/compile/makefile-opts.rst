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
  endpoint - for UART output, see the ``UART_LOGGING`` flag.

- ``UART_LOGGING`` - When combined with ``DEBUG``, set to ``1`` to enable debug
  logging via UART. See the :doc:`platform docs </platforms/platforms>` for
  details on how to read this output.

- ``LOG_STATS`` - Set to ``1`` to enable logging CAN message and output message
  statistics over the normal DEBUG output.

- ``DEFAULT_CAN_ACK_STATUS`` - Set to ``1`` to enable write mode on the CAN
  controllers by default, so messages are ACKed. See the :doc:`testing section </testing>`
  for more details.

- ``NETWORK`` - By default, TCP output of OpenXC vehicle data is disabled. Set
  this to ``1`` to enable TCP output on boards that have an Network interface
  (only the chipKIT Max32 right now). (Note that the NETWORK option is broken on
  the chipKIT Max32 build for the moment, see https://github.com/openxc/vi-firmware/issues/189.

- ``NETWORK_ALLOW_RAW_WRITE`` - By default, raw CAN message write requests are
  not allowed from the network interface even if the CAN bus is configured to
  allow raw writes - set this to ``1`` to accept them.

- ``BLUETOOTH_ALLOW_RAW_WRITE`` - By default, raw CAN message write requests are
  not allowed from the Bluetooth interface even if the CAN bus is configured to
  allow raw writes - set this to ``1`` to accept them.

- ``USB_ALLOW_RAW_WRITE`` - By default, raw CAN message write requests *are*
  allowed from the wired USB interface (if the CAN bus is also configured to
  allow raw writes) - set this to ``0`` to block them.

- ``BINARY_OUTPUT`` - By default, the output format is JSON. Set this to ``1``
  to use a binary output format, described more in :doc:`/advanced/binary`.

- ``INCLUDE_RECURRING_OBD2_REQUESTS`` - Define this flag (e.g. set it to ``1``)
  to include a set of recurring OBD-II requests in the build, to be requests
  immediately on startup.

- ``BOOTLOADER`` - By default, the firmware is built to run on a microcontroller
  with a bootloader (if one is available for the selected platform), allowing
  you to update the firmware without specialized hardware. If you want to build
  to run on bare-metal hardware (i.e. start at the top of flash memory) set this
  to ``0``.
