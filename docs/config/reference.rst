===================================
All Configuration Options Reference
===================================

There are many configuration options - we recommend looking for your use case in
the list of :doc:`read <examples>` or :doc:`write <write-examples>` to find a
starting point, and refer to this section as a reference on particular
configuration options.

The root level JSON object maps CAN bus addresses to CAN bus objects,  CAN
message IDs to CAN message objects in each bus, and CAN signal name to signal
object within each message. Other top-level sections are available to list
one-time initialization functions and to list arbitrary functions that should be
added to the main loop.

In all cases when we refer to a "path," either an absolute or relative
path will work. If you use relative paths, however, they must be relative
to the root of wherever you run the build scripts.

Once you've defined your message set in a JSON file, run the
``openxc-generate-firmware-code`` tool from the `OpenXC Python library
<http://python.openxcplatform.com>`_ to create an implementation of
``signals.cpp``:

.. code-block:: sh

    vi-firmware/ $ openxc-generate-firmware-code --message-set mycar.json > src/signals.cpp

.. contents::
    :local:
    :depth: 1

Message Set
============

Each JSON mapping file defines a "message set," and it should have a name.
Typically this identifies a particular model year vehicle, or possibly a broader
vehicle platform. The ``name`` field is required.

``bit_numbering_inverted`` (optional)
  This flag controls the default :ref:`bit numbering <bit-numbering>` for all
  messages included in this message set. You can override the bit numbering for
  any particular message or mapping, too.

  ``false`` by default, ``true`` by default for database-backed
  mappings.

``max_message_frequency``
  Set a default value for all buses for this attribute - see the :ref:`Can Buses
  <canbus>` section for a description.

``raw_can_mode``
  Set a default value for all buses for this attribute - see the :ref:`Can Buses
  <canbus>` section for a description.

Parent Message Sets
===================

Message sets are composable - you can extend a set by adding a path to the
parent(s) to the ``parents`` key.

.. _initializer:

Initializers
============

The key ``initializers`` should have as its value an array of strings. Each
string should be the name of a function with the type signature:

.. code-block:: c

   void function();

These functions will be called once at the beginning of execution, before
reading any CAN messages.

.. _looper:

Loopers
=======

The key ``loopers`` should have as its value an array of strings. Each
string should be the name of a function with the type signature:

.. code-block:: c

   void function();

These functions will be called once each time through the main loop function,
after reading and processing any CAN messages.

.. _canbus:

CAN Buses
=========

The key ``buses`` must be an object, where each field is a CAN bus uses by this
message set, and which CAN controllers are attached on the microcontroller. The

``controller``
  The integer ID of the CAN controller to which this bus is attached. The
  platforms we are using now only have 2 CAN controllers, identified here by
  ``1`` and ``2`` - these are the only acceptable bus addresses. If this field
  is not defined, the bus and any messages associated with it will be ignored
  (but it won't cause an error, so you can swap between buses very quickly).

``speed``
  The CAN bus speed in Kbps, most often 125000 or 500000.

``raw_can_mode``
  Controls sending raw CAN messages (encoded as JSON objects) from the bus over
  the output channel. Valid modes are ``off`` (the default if you don't specify
  this attribute), ``filtered`` (if messages are defined for the bus, will
  enable CAN filters and only transmit those messages), or ``unfiltered``
  (disable acceptance filters and send all received CAN messages). If this
  attribute is set on a CAN bus object, it will override any default set at the
  message set level (e.g. you can have all buses configured to send ``filtered``
  raw CAN messages, but override one to send ``unfiltered``).

``raw_writable``
  Controls whether or not raw CAN messages from the user can be written back to
  this bus, without any sort of translation. This is false by default. Even when
  this is false, messages may still be written to the bus if a signal is
  configured as ``writable``, but they will translated from the user's input
  first.

``max_message_frequency``
  The default maximum frequency for all CAN messages when using the raw
  passthrough mode. To put no limit on the frequency, set this to 0 or leave it
  out. If this attribute is set on a CAN bus object, it will override any
  default set at the message set level. This value cascades to all CAN message
  objects for their ``max_frequency`` attribute, which can also be overridden at
  the message level.

``force_send_changed`` (optional)
  Meant to be used in conjunction with ``max_message_frequency``, if this is
  true a raw CAN message will be sent regardless of the given frequency if the
  value has changed (when using raw CAN passthrough). Setting the value here, on
  the CAN bus object, will cascade down to all CAN messages unless overridden.
  Defaults to ``true``.

.. _messages:

CAN Messages
============

The ``messages`` key is a object with fields mapping from CAN message IDs
to signal definitions. The fields must be hex IDs of CAN messages as
strings (e.g. ``0x90``).

Message
-------

The attributes of each message object are:

``bus``
  The name of one of the previously defined CAN buses where this message can be
  found.

``bit_numbering_inverted`` (optional)
  This flag controls the default :ref:`bit numbering <bit-numbering>` for the
  signals in this message. Defaults to the value of the mapping, then default of
  the message set.

``signals``
  A list of CAN signal objects (described in the :ref:`signal` section) that are
  in this message, with the name of the signal as the key. If this is a
  database-backed mappping, this value must match the signal name in the
  database exactly - otherwise, it's an arbitrary name.

``name`` (optional)
  The name of the CAN message - this is not required and has no meaning in code,
  it can just be handy to be able to refer back to an original CAN message
  definition in another document.

``handlers`` (optional)
  An array of names of functions that will be compiled with the firmware and
  should be applied to the entire raw message value (see
  :ref:`message-handlers`).

``enabled`` (optional)
  Enable or disable all processing of a CAN message. By default, a message is
  enabled. If this flag is false, the CAN message and all its signals will be
  left out of the generated source code. Defaults to ``true``.

``max_frequency`` (optional)
  If sending raw CAN messages to the output interfaces,
  this controls the maximum frequency (in Hz) that the message will be process
  and let through. The default value (``0``) means that all messages will be
  processed, and there is no limit imposed by the firmware. If you want to make
  sure you don't miss a change in value even when rate limiting, see the
  ``force_send_changed`` attribute. Defaults to 0 (no limit).

``max_signal_frequency`` (optional)
  Setting the max signal frequency at the message level will cascade down to all
  of the signals within the message (unless overridden). The default value
  (``0``) means that all signals will be processed, and there is no limit
  imposed by the firmware. See the ``max_frequency`` flag documentation for the
  signal mapping for more information. If you want to make sure you don't miss a
  change in value even when rate limiting, see the
  ``force_send_changed_signals`` attribute. Defaults to 0 (no limit).

``force_send_changed`` (optional)
  Meant to be used in conjunction with ``max_frequency``, if this is true a raw
  CAN message will be sent regardless of the given frequency if the value has
  changed (when using raw CAN passthrough). Defaults to ``true``.

``force_send_changed_signals``
  Setting this value on a message will cascade down to all of the signals within
  the message (unless overridden). See the ``force_send_changed`` flag
  documentation for the signal mapping for more information. Defaults to
  ``false``.

.. _message-handlers:

Message Handlers
----------------

If you need additional control, you can provide custom handlers for the entire
message to combine multiple signals into a single value (or any other arbitrary
processing). You can generate 0, 1 or many translated messages from each call to
a custom handler function.

.. code-block:: c

    void handleSteeringWheelMessage(int messageId, uint64_t data,
            CanSignal* signals, int signalCount, Pipeline* pipeline);
        float steeringWheelAngle = decodeCanSignal(&signals[1], data);
        float steeringWheelSign = decodeCanSignal(&signals[2], data);

        float finalValue = steeringWheelAngle;
        if(steeringWheelSign == 0) {
            // left turn
            finalValue *= -1;
        }

        char* message = generateJson(signals[1], finalValue);
        sendMessage(usbDevice, (uint64_t*) message, strlen(message));
    }

Using a custom message handler will not automatically stop the normal
translation workflow for individual signals. To mute them (but still store
their values in ``signal->lastvalue``), specify ``ignoreHandler`` as the
``handler``. This is not done by default because not every signal in
a message is always handled by a message handler.

.. _signal:

Signal
-------

The attributes of a ``signal`` object within a ``message`` are:

``generic_name``
  The name of the associated generic signal name (from the OpenXC specification)
  that this should be translated to. Optional - if not specified, the signal is
  read and stored in memory, but not sent to the output bus. This is handy for
  combining the value of multiple signals into a composite measurement such as
  steering wheel angle with its sign.

``bit_position``
  The starting bit position of this signal within the message. Required unless
  this is a database-backed mapping.

``bit_size``
  The width in bits of the signal. Required unless this is a database-backed
  mapping.

``factor``
  The signal value is multiplied by this if set. Required unless this is a
  database-backed mapping.

``offset``
  This is added to the signal value if set. Required unless this is a
  database-backed mapping.

``decoder`` (optional)
  The name of a function that will be compiled with the firmware and should be
  applied to the signal's value after the normal translation. See the
  :ref:`signal-decoders` section for details.

``ignore`` (optional)
  Setting this to ``true`` on a signal will silence output of the signal. The VI
  will not monitor the signal nor store any of its values. This is useful if you
  are using a custom decoder for an entire message, want to silence the normal
  output of the signals it handles, *and* you don't need the VI to keep track of
  the values of any of the signals separately (in the ``lastValue`` field). If
  you need to use the previously stored values of any of the signals, you can
  use the ``ignoreDecoder`` as the decoder for the signal. Defaults to
  ``false``.

``enabled`` (optional)
  Enable or disable all processing of a CAN signal. By default, a signal is
  enabled; if this flag is false, the signal will be left out of the generated
  source code. Defaults to ``true``.

The difference between ``ignore``, ``enabled`` and using an ``ignoreDecoder``
can be confusing. To summarize the difference:

* The ``enabled`` flag is the master control switch for a signal - when this is
  false, the signal (or message, or mapping) will not be included in the
  firmware at all. A common time to use this is if you want to have
  one configuration file with many options, only a few of which are enabled in
  any particular build.
* The ``ignore`` flag will not exclude a signal from the firmware, but it will
  not include it in the normal message processing pipeline. The most common use
  case is when you need to reference the bit field information for the signal
  from a custom decoder.
* Finally, use the ``ignoreDecoder`` for your signal's ``decoder`` to both
  include it in the firmware and handle it during the normal message processing
  pipeline, but just silence its output. This is useful if you need to track the
  last known value for this signal for a calculation in a custom decoder.

``states``
  This is a mapping between the desired descriptive states (e.g. ``off``) and
  the corresponding numerical values from the CAN message (usually an integer).
  The raw values are specified as a list to accommodate multiple raw states
  being coalesced into a single final state (e.g. key off and key removed both
  mapping to just "off"). Required unless this is a database-backed mapping.

``max_frequency`` (optional)
  Some CAN signals are sent at a very high frequency, likely more often than
  will ever be useful to an application. This attribute sets the maximum
  frequency (Hz) that the signal will be processed and let through. The default
  value (``0``) means that all values will be processed, and there is no limit
  imposed by the firmware. If you want to make sure you don't miss a change in
  value even when dropping messages, see the ``force_send_changed`` attribute.
  You probably don't want to combine this attribute with ``send_same`` or else
  you risk missing a status change message if wasn't one of the messages the VI
  decided to let through. Defauls to 0 (no limit).

``send_same`` (optional)
  By default, all signals are translated every time they are received from the
  CAN bus. By setting this to ``false``, you can force a signal to be sent only
  if the value has actually changed. This works best with boolean and state
  based signals. Defaults to ``true``.

``force_send_changed`` (optional)
  Meant to be used in conjunction with ``max_frequency``, if this is true a
  signal will be sent regardless of the given frequency if the value has
  changed. This is useful for state-based and boolean states, where the state
  change is the most important thing and you don't want that message to be
  dropped. Defaults to ``false``.

``writable`` (optional)
  Set this attribute to ``true`` to allow this signal to be written back to the
  CAN bus by an application. OpenXC JSON-formatted messages sent back to the VI
  that are writable are translated back into raw CAN messages and written to the
  bus. By default, the value will be interpreted as a floating point number.
  Defaults to ``false``.

``encoder`` (optional)
  If the signal is writable and is not a plain floating point number (i.e. it is
  a boolean or state value), you can specify a custom function here to encode
  the value for a CAN messages. This is only necessary for boolean types at the
  moment - if your signal has states defined, we assume you need to encode a
  string state value back to its original numerical value. Defaults to a
  built-in numerical encoder.

.. _signal-decoders:

Signal Decoder
--------------

The default decoder for each signal is a simple passthrough, translating the
signal's value from engineering units to something more usable (using the
defined factor and offset). Some signals require additional processing that you
may wish to do within the VI and not on the host device. Other signals may need
to be combined to make a composite signal that's more meaningful to developers.

An good example is steering wheel angle. For an app developer to get a
value that ranges from e.g. -350 to +350, we need to combine two
different signals - the angle and the sign. If you want to make this
combination happen inside the VI, you can use a custom decoder.

You may also need a custom decoder to return a value of a type other than float.
A decoder is provided for dealing with boolean values, the ``booleanDecoder`` -
if you specify that as your signal's ``decoder`` the resulting JSON will contain
``true`` for 1.0 and ``false`` for 0.0. There is also a ``stateDecoder`` for
translating integer state values to string names.

For this example, we want to modify the value of ``steering_wheel_angle``
by setting the sign positive or negative based on the value of the other
signal (``steering_angle_sign``). Every time a CAN signal is received, the
new value is stored in memory. Our custom decoder
``decodeSteeringWheelAngle`` will use that to adjust the raw steering
wheel angle value. Modify the input JSON file to set the ``decoder``
attribute for the steering wheel angle signal to
``decodeSteeringWheelAngle``.

Add this to the top of ``signals.cpp`` (or if using the mapping file, add it to
a separate ``.cpp`` file and then add that filename to the ``extra_sources``
field):

.. code-block:: c

    openxc_DynamicField decodeSteeringWheelAngle(CanSignal* signal,
            CanSignal* signals, int signalCount,
            openxc::pipeline::Pipeline* pipeline,
            float value, bool* send) {
        if(signal->lastValue == 0) {
            // left turn
            value *= -1;
        }
        return openxc::payload::wrapNumber(value);
    }

The function declaration of a custom decoder must match:

.. code-block:: c

    openxc_DynamicField customDecoder(CanSignal* signal, CanSignal* signals,
        int signalCount, openxc::pipeline::Pipeline* pipeline,
            float value, bool* send);

where ``signal`` is a pointer to the ``CanSignal`` this is handling,
``signals`` is an array of all signals, ``value`` is the raw value
from CAN and ``send`` is a flag to indicate if this should be sent over
USB.

The ``bool* send`` parameter is a pointer to a ``bool`` you can flip to
``false`` if this signal value need not be sent over USB. This can be
useful if you don't want to keep notifying the same status over and over
again, but only in the event of a change in value (you can use the
``lastValue`` field on the CanSignal object to determine if this is true).
It's also good practice to inspect the value of ``send`` when your custom
decoder is called - the normal translation workflow may have decided the
data shouldn't be sent (e.g. the value hasn't changed and ``sendSame ==
false``). Decoders are called every time a signal is received, even if
``send == false``, so that you have the flexibility to implement custom
processing that depends on receiving every data point.

A known issue with this method is that there is no guarantee that the
last value of another signal arrived in the message or before/after the
value you're current modifying. For steering wheel angle, that's
probably OK - for other signals, not so much.

.. _diagnostic-messages:

Diagnostic Messages
===================

The ``diagnostic_messages`` key is an array of objects describing a recurring
diagnostic message request.

Diagnostic Message
------------------

The attributes of each diagnostic message object are:

``bus``
  The name of one of the previously defined CAN buses where this message should be requested.

``id``
  the arbitration ID for the request.

``mode``
  The diagnostic request mode, e.g. Mode 1 for powertrain diagnostic requests.

``frequency``
  The frequency in Hz to request this diagnostic message. The maximum allowed
  frequency is 10Hz.

``pid`` (optional)
  If the mode uses PIDs, the pid to request.

``name`` (optional)
  A human readable, string name for this request. If provided, the response will
  have a ``name`` field (much like a normal translated message) with this value
  in place of ``bus``, ``id``, ``mode`` and ``pid``.

``decoder`` (optional)
  When using a ``name``, you can also specify a custom decoder function to parse
  the payload. This field is the name of a function (that matches the
  ``DiagnosticResponseDecoder`` function prototype). When a decoder is
  specified, the decoded value will be returned in the ``value`` field in place
  of ``payload``.

``callback`` (optional)
  This field is the name of a function (that matches the
  ``DiagnosticResponseCallback`` function prototype) that should be called every
  time a response is received to this request.

Mappings
========

The ``mappings`` field is an optional field allows you to move the definitions
from the ``messages`` list to separate files for improved composability and
readability.

For an detailed explanation of mapped message sets, see the example of a message
set using mappings, see the :ref:`mapped` configuration example.

The ``mappings`` field must be a list of JSON objects with:

``mapping`` -
  A path to a JSON file containing a single object with the key ``messages``,
  containing objects formatted as the :ref:`Messages` section describes. In
  short, you can pull out the ``messages`` key from the main file and throw it
  into a separate file and link it in here. You can also do the same with a
  ``diagnostic_messages`` field containing :ref:`diagnostic-messages`.

``bus`` (optional)
  The name of one of the defined CAN buses where these messages can be found -
  this value will be set for all of the messages contained the mapping file, but
  can be overridden by setting ``bus`` again in an individual message.

``database`` (optional)
  A path to a CAN message database associated with these mappings. Right now,
  XML exported from Vector CANdb++ is supported. If this is defined, you can
  leave the bit position, bit size, factor, offset, max and min values out of
  the ``mapping`` file - they will be picked up automatically from the database.

``bit_numbering_inverted`` (optional)
  This flag controls the default :ref:`bit numbering <bit-numbering>` for the
  messages contained in this mapping. Messages in the mapping can override the
  bit numbering by explicitly specifying their own value for this flag. Defaults
  to the value of the message set, or ``true`` if this mapping is
  database-backed.

``enabled`` (optional)
  Enable or disable all processing of the CAN messages in a mapping. By default,
  a mapping is enabled; if this flag is false, all CAN message and signals from
  the mapping will be excluded from the generated source code. Defaults to
  ``true``.

Extra Sources
=============

The ``extra_sources`` key is an optional list of C++ source files that should be
injected into the generated ``signals.cpp`` file. These may include signal
decoders, message handlers, initializers or custom loopers.

Commands
========

The ``commands`` field is a mapping of arbitrary command names to functions that
should be called to run arbitrary code in the VI on-demand (e.g. sending
multiple CAN signals at once). The value of this attribute is a list of objects
with these attributes:

``name``
  The name of the command to be recognized on the OpenXC translated interface.

``enabled`` (optional)
  Enable or disable all processing of a command. By default, a command is
  enabled. If this flag is false, the command will be excluded from the
  generated source code. Defaults to ``true``.

``handler``
  The name of a custom command handler function (that matches the
  ``CommandHandler`` function prototype from ``canutil.h``) that should be
  called when the named command arrives over the translated VI interface (e.g.
  USB or Bluetooth).

.. code-block:: c

  void (*CommandHandler)(const char* name, openxc_DynamicField* value,
          openxc_DynamicField* event, CanSignal* signals, int signalCount);

Any message received from the USB host with that given command name will be
passed to your handler. This is useful for situations where there isn't a 1 to
1 mapping between OpenXC command and CAN signal, e.g. if the left and right turn
signal are split into two signals instead of the 1 state-based signal used by
OpenXC. You can use the ``sendCanSignal`` function in ``canwrite.h`` to do the
actual data sending on the CAN bus.
