==================================
Diagnostic Configuration Examples
==================================

The firmware configuration examples shown so far are for so-called "normal mode"
CAN messages, that are sent and received without any formal request by a node on
the bus. Diagnostic type messages use a request / response style protocol, and
are also supported by the VI firmware. You can add pre-defined, diagnostic
recurring requests to a VI config file (e.g. request the engine RPM once per
second).

To perform a one-time request, you don't need anything special in the
configuration - just use a :ref:`diagnostic request command
<vehicle-diagnostic-requests>`.

.. contents::
    :local:
    :depth: 1

Recurring Simple Diagnostic PID Request
========================================

We want to send a request for a mode 0x22 PID once per second and publish the
full details of the response out as an OpenXC message.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "raw_writable": true,
               "speed": 500000
           }
       },
       "diagnostic_messages": [
           {
               "bus": "hs",
               "id": 2015,
               "mode": 34,
               "pid": 42,
               "frequency": 1
           }
       ]
   }

We added a ``diagnostic_messages`` array and one diagnostic request object
in that array. The ``id``, ``mode``, and ``frequency`` are the only required
fields, and we added a ``pid`` to this as well.

It's also important that the CAN controller is configured as writable with the
``raw_writable`` flag, otherwise the VI will not be able to send the diagnostic
request.

With this configuration, the VI will publish the diagnostic response received
from the bus using the
`OpenXC diagnostic response message format
<https://github.com/openxc/openxc-message-format#responses>`_, e.g. when
using the JSON output format:

.. code-block:: javascript

    {"bus": 1,
      "id": 2016,
      "mode": 34,
      "pid": 42,
      "success": true,
      "payload": "0x1234"}

Note that the ``id`` of the response is different from the request, because we
sent the request to the functional broadcast address ``0x7df``. If you use a
physical address with your request, the ID of the response will match. The
``bus`` is also different from that specified in the mapping - it's the
controller ID, not the name of the bus.

Recurring Parsed OBD-II PID Request
========================================

We want to send a request for the OBD-II PID for engine RPM once per second, and
publish the full details of the response out as an OpenXC message. We want to
use a decoder to parse the response's payload to get the actual engine RPM
value.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "raw_writable": true,
               "speed": 500000
           }
       },
       "diagnostic_messages": [
           {
               "bus": "hs",
               "id": 2015,
               "mode": 1,
               "pid": 12,
               "frequency": 1,
               "decoder": "handleObd2Pid"
           }
       ]
   }

Besides changing the ``mode`` and ``pid``, we added a ``decoder``. The
``handleObd2Pid`` decoder is included by default in the vi-firmware repository,
and knows how to decode a number of the most interesting and widely implemented
OBD-II PIDs.

With this configuration, the VI will publish the diagnostic response received
from the bus using the
`OpenXC diagnostic response message format
<https://github.com/openxc/openxc-message-format#responses>`_, e.g. when
using the JSON output format:

.. code-block:: javascript

    {"bus": 1,
      "id": 2016,
      "mode": 34,
      "pid": 42,
      "success": true,
      "payload": "0x1234"}

Unlike the configuration example without a ``decoder``, this response has a
``value`` instead of the raw ``payload``. The value is whatever your ``decoder``
function returns.

Recurring Named Diagnostic PID Request
========================================

Just like before, we want to request the OBD-II PID for engine RPM once per
second, but this time we don't care about returning the full details in the
response message. We just want a named message like an OpenXC simple vehicle
message.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "raw_writable": true,
               "speed": 500000
           }
       },
       "diagnostic_messages": [
           {
               "bus": "hs",
               "id": 2015,
               "mode": 1,
               "pid": 12,
               "frequency": 1,
               "decoder": "handleObd2Pid",
               "name": "engine_speed"
           }
       ]
   }

We simply added a ``name`` field to the diagnostic message configuration. This
will change the output format to the
`OpenXC simple vehicle message format
<https://github.com/openxc/openxc-message-format/blob/next/JSON.mkd#simple-vehicle-message>`_,
e.g. when using the JSON output format:

.. code-block:: js

    {"name": "engine_speed", "value": 45}

where ``value`` is the return value from the ``decoder``.

On-Demand Request from Command Handler
======================================

You can generate a new recurring or one-off diagnostic request from any custom
command handler signal decoder, or CAN message handler. Take a look at the
``diagnostics.h`` module for functions that may be useful.

For this example, we want to generate a mode 3 diagnostic request to get trouble
codes when a "collect_trouble_codes" command is sent. We will register a
callback function to handle the payload of the response to parse out the trouble
code we are looking for. Here's our VI config:

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "raw_writable": true,
               "speed": 500000
           }
       },
       "commands": [
           {"name": "collect_trouble_codes",
            "handler": "collectTroubleCodes"}
       ],
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

Just as in the :ref:`command-example`, we added a ``commands`` field with one
custom command, mapping ``collect_trouble_codes`` to the command handler
function ``collectTroubleCodes`` (to be defined in ``my_handlers.cpp``).

In ``my_handlers.cpp``:

.. code-block:: cpp

   void handleTroubleCodeResponse(
            DiagnosticsManager* manager,
            const ActiveDiagnosticRequest* request,
            const DiagnosticResponse* response,
            float parsed_payload) {
       // Received a response to our mode 3 request

       // response->payload is an array (with length response->payload_length)
       // that contains the trouble code data - do whatever you need to do to parse
       // out your trouble codes.

       // If you need to send anything out on the I/O interfaces (e.g. to let
       // a client know about a particular trouble code), you can use the
       // openxc::pipeline::publish(...) function.
   }

   void handleMyCommand(const char* name, openxc_DynamicField* value,
         openxc_DynamicField* event, CanSignal* signals, int signalCount) {

      // Build and broadcast a non-recurring mode 3 diagnostic request
      DiagnosticRequest request = {
          arbitration_id: 0x7df,
          mode: 3
      };

      addRequest(&getConfiguration()->diagnosticsManager,
         // use the CAN bus on controller 0 (this is a little bit dangerous,
         // you'll want to do some error checking to amke sure this bus exists.
         getCanBuses()[0],
         &request,
         NULL, // no human readable name
         false, // don't wait for multiple responses
         NULL, // no response decoder
         handleTroubleCodeResponse); // when a response is received, call our handler
   }

This combination of a command handler and diagnostic response callback requests
trouble codes from the vehicle whenever the command is received, and can take
any action on the response (in the callback.
