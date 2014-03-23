==================================
Diagnostic Configuration Examples
==================================

The firmware configuration examples shown so far are for so-called "normal mode"
CAN messages, that are sent and received without any formal request by a node on
the bus. Diagnostic type messages use a request / response style protocol, but
you can add them to a VI config file to create a recurring request (e.g. request
the engine RPM once per second).

To perform a one-time request, you don't need anything special in the
configuration. Make sure the CAN bus is configured as ``raw_writable`` (the
default build is correct), and then use one of the support library tools like
``openxc-diag`` to send a request.

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

The response will look like:

.. code-block:: javascript

    {"bus": 1,
      "id": 2016,
      "mode": 34,
      "pid": 42,
      "success": true,
      "payload": "0x1234"}

Note that the ``id`` of the response is different from the request, because we
sent the request to the functional broadcast address ``0x7df``. If you use a
physical address with your request, the ID of the response will match.

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

The response will look like:

.. code-block:: javascript

    {"bus": 1,
      "id": 2016,
      "mode": 1,
      "pid": 12,
      "success": true,
      "value": 412}

Unlike the configuration example without a ``decoder``, this response has a
``value`` instead of the raw ``payload``. The value is whatever your ``decoder``
function returns.

Recurring Named Diagnostic PID Request
========================================

Just like before, we want to request the OBD-II PID for engine RPM once per
second, but this time we don't care about returning the full details in the
response message. We just want a named message like the OpenXC "translated"
message type.

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
will change the output format for the response to:

::

    {"name": "engine_speed", "value": 45}

where ``value`` is the return value from the ``decoder``.
