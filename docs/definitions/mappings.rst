==========================
JSON Mapping Format
==========================

The code generation script (to generate ``signals.cpp``) requires a JSON file of
a specific format as input. The input format is a JSON object like the one found
in `signals.json.example
<https://github.com/openxc/cantranslator/blob/master/src/signals.json.example>`_.

The root level JSON object maps CAN bus addresses to CAN bus objects,  CAN
message IDs to CAN message objects in each bus, and CAN signal name to signal
object within each message. Other top-level sections are available to list
one-time initialization functions and to list arbitrary functions that should be
added to the main loop.

In all cases when we refer to a "path," either an absolute or relative
path will work. If you use relative paths, however, they must be relative
to the root of wherever you run the build scripts.

Message Set Name
================

Each JSON mapping file defines a "message set," and it should have a name.
Typically this identifies a particular model year vehicle, or possibly a broader
vehicle platform. The ``name`` field is required.

``bit_numbering_inverted`` - (optional, ``true`` by default) This flag controls
the default :ref:`bit numbering <bit-numbering>` for all messages included in this message set.
You can override the bit numbering for any particular message or mapping, too.

Parent Message Sets
===================

Message sets are composable - you can extend a set by adding a path to the
parent(s) to the ``parents`` key.

Initializers
============

The key ``initializers`` should have as its value an array of strings. Each
string should be the name of a function with the type signature:

.. code-block:: c

   void function();

These functions will be called once at the beginning of execution, before
reading any CAN messages.

Loopers
=======

The key ``loopers`` should have as its value an array of strings. Each
string should be the name of a function with the type signature:

.. code-block:: c

   void function();

These functions will be called once each time through the main loop function,
after reading and processing any CAN messages.

CAN Buses
=========

The key ``buses`` must be an object, where each field is a CAN bus uses by this
message set, and which CAN controllers are attached on the microcontroller. The

``controller`` - The integer ID of the CAN controller to which this bus is
attached. The platforms we are using now only have 2 CAN controllers, identified
here by ``1`` and ``2`` - these are the only acceptable bus addresses. If this
field is not defined, the bus and any messages associated with it will be
ignored (but it won't cause an error, so you can swap between buses very
quickly).

``speed`` - The CAN bus speed in Kbps, most often 125000 or 500000.

.. _messages:

CAN Messages
============

The ``messages`` key is a object with fields mapping from CAN message IDs
to signal definitions. The fields must be hex IDs of CAN messages as
strings (e.g. ``0x90``).

Message
-------

The attributes of each message object are:

``bus`` - The name of one of the previously defined CAN buses where this message
can be found.

``bit_numbering_inverted`` - (optional, defaults to the value of the message set)
This flag controls the default :ref:`bit numbering <bit-numbering>` for the signals in this message.

``signals`` - A list of CAN signal objects (described in the :ref:`signal`
section) that are in this message, with the name of the signal as the key. If
this is a database-backed mappping, this value must match the signal name in the
database exactly - otherwise, it's an arbitrary name.

``name`` - (optional) The name of the CAN message - this is not required and has
no meaning in code, it can just be handy to be able to refer back to an original
CAN message definition in another document.

``handler`` - (optional) The name of a function that will be compiled with the
firmware and should be applied to the entire raw message value (see
:ref:`message-handlers`).

``enabled`` - (optional, true by default) Enable or disable all processing of a
CAN message. By default, a message is enabled. If this flag is false, the CAN
message and all its signals will be left out of the generated source code.

.. _message-handlers:

Message Handlers
----------------

If you need additional control, you can provide a custom handler for the
entire message to combine multiple signals into a single value (or any
other arbitrary processing). You can generate 0, 1 or many translated
messages from one call to your handler function.

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
``value_handler``. This is not done by default because not every signal in
a message is always handled by a message handler.

.. _signal:

Signal
-------

The attributes of a ``signal`` object within a ``message`` are:

``generic_name`` - The name of the associated generic signal name (from
the OpenXC specification) that this should be translated to. Optional -
if not specified, the signal is read and stored in memory, but not sent
to the output bus. This is handy for combining the value of multiple
signals into a composite measurement such as steering wheel angle with
its sign.

``bit_position`` - (required only if not a database-backed mapping) The staring
bit position of this signal within the message.

``bit_size`` - (required only if not a database-backed mapping) The width in
bits of the signal.

``factor`` - (required only if not a database-backed mapping) The signal value
is multiplied by this if set. Optional.

``offset`` - (required only if not a database-backed mapping) This is added to
the signal value if set. Optional.

``handler`` - (optional) The name of a function that will be compiled with the
firmware and should be applied to the signal's value after the normal
translation. See the :ref:`value-handlers` section for details.

``ignore`` - (default: false) Setting this to ``true`` on a signal will silence
output of the signal. The VI will not monitor the signal nor store any of its
values. This is useful if you are using a custom handler for an entire message,
want to silence the normal output of the signals it handles, *and* you don't
need the VI to keep track of the values of any of the signals separately (in the
``lastValue`` field). If you need to use the previously stored values of any of
the signals, you can use the ``ignoreHandler`` as a value handler for the
signal.

``states`` - (required only for state-based signals) This is a mapping between
the desired descriptive states (e.g. ``off``) and the corresponding numerical
values from the CAN message (usually an integer). The raw values are specified
as a list to accommodate multiple raw states being coalesced into a single final
state (e.g. key off and key removed both mapping to just "off").

``send_frequency`` - (default: 1) Some CAN signals are sent at a very high
frequency, likely more often than will ever be useful to an application. The
value of this attribute is used as the denominator in the ratio ``1/x`` to
determine the percentage of the signals that will be let through. The default
value (``1``) means that ``1/1`` (i.e. 100%) of the signal values received will
be translated. Increasing the value will reduce the number of messages that are
sent - a value of ``10`` means that only ``1/10`` messages (i.e. every 10th
message) is processed. You don't want to combine this attribute with
``send_same`` or else you risk missing a status change message if wasn't one of
the messages the VI decided to let through.

``send_same`` - (default: ``true``) By default, all signals are translated every
time they are received from the CAN bus. By setting this to ``false``, you can
force a signal to be sent only if the value has actually changed. This works
best with boolean and state based signals.

``writable`` - (default: ``false``) Set this attribute to ``true`` to allow this
signal to be written back to the CAN bus by an application. OpenXC
JSON-formatted messages sent back to the VI that are writable are translated
back into raw CAN messages and written to the bus. By default, the value will be
interpreted as a floating point number.

``write_handler`` - (optional, default is a numerical handler) If the signal is
writable and is not a plain floating point number (i.e. it is a boolean or state
value), you can specify a custom function here to encode the value for a CAN
messages. This is only necessary for boolean types at the moment - if your
signal has states defined, we assume you need to encode a string state value
back to its original numerical value.

``enabled`` - (optional, true by default) Enable or disable all processing of a
CAN signal. By default, a signal is enabled; if this flag is false, the signal
will be left out of the generated source code.

.. _value-handlers:

Value Handlers
--------------

The default value handler for each signal is a simple passthrough,
translating the signal's value from engineering units to something more
usable (using the defined factor and offset). Some signals require
additional processing that you may wish to do within the VI and
not on the host device. Other signals may need to be combined to make a
composite signal that's more meaningful to developers.

An good example is steering wheel angle. For an app developer to get a
value that ranges from e.g. -350 to +350, we need to combine two
different signals - the angle and the sign. If you want to make this
combination happen inside the VI, you can use a custom handler.

You may also need a custom handler to return a value of a type other
than float. A handler is provided for dealing with boolean values, the
``booleanHandler`` - if you specify that as your signal's
``value_handler`` the resulting JSON will contain ``true`` for 1.0 and
``false`` for 0.0. If you want to translate integer state values to
string names (for parsing as an enum, for example) you will need to
write a value handler that returns a ``char*``.

For this example, we want to modify the value of ``steering_wheel_angle``
by setting the sign positive or negative based on the value of the other
signal (``steering_angle_sign``). Every time a CAN signal is received, the
new value is stored in memory. Our custom handler
``handleSteeringWheelAngle`` will use that to adjust the raw steering
wheel angle value. Modify the input JSON file to set the ``value_handler``
attribute for the steering wheel angle signal to
``handleSteeringWheelAngle``.

Add this to the top of ``signals.cpp`` (or if using the mapping file, add it to
a separate ``.cpp`` file and then add that filename to the ``extra_sources``
field):

.. code-block:: c

    float handleSteeringWheelAngle(CanSignal* signal, CanSignal* signals,
            int signalCount, float value, bool* send) {
        if(signal->lastValue == 0) {
            // left turn
            value *= -1;
        }
        return value;
    }

The valid return types for value handlers are ``bool``, ``float`` and
``char*`` - the function prototype must match one of:

.. code-block:: c

    char* customHandler(CanSignal* signal, CanSignal* signals, int signalCount,
            float value, bool* send);

    float customHandler(CanSignal* signal, CanSignal* signals, int signalCount,
            float value, bool* send);

    bool customhandler(cansignal* signal, cansignal* signals, int signalCount,
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
handler is called - the normal translation workflow may have decided the
data shouldn't be sent (e.g. the value hasn't changed and ``sendSame ==
false``). Handlers are called every time a signal is received, even if
``send == false``, so that you have the flexibility to implement custom
processing that depends on receiving every data point.

A known issue with this method is that there is no guarantee that the
last value of another signal arrived in the message or before/after the
value you're current modifying. For steering wheel angle, that's
probably OK - for other signals, not so much.

Mappings
========

The ``mappings`` field is an optional field allows you to move the definitions
from the ``messages`` list to separate files for improved composability and
readability.

For an example of a message set using mappings, see the
`mapped-signals.json.example
<https://github.com/openxc/cantranslator/blob/master/src/mapped-signals.json.example>`_
file in the repository.

The ``mappings`` field must be a list of JSON objects with:

``mapping`` - a path to a JSON file containing a single object with the key
``messages``, containing objects formatted as the :ref:`Messages` section
documents. In short, you can pull out the ``messages`` key from the main file
and throw it into a separate file and link it in here.

``bus`` - (optional) The name of one of the defined CAN buses where these
messages can be found - this value will be set for all of the messages contained
the mapping file, but can be overridden by setting ``bus`` again in an individual
message.

``database`` - (optional) a path to
a CAN message database associated with these mappings. Right now, XML exported
from Vector CANdb++ is supported. If this is defined, you can leave the bit
position, bit size, factor, offset, max and min values out of the ``mapping``
file - they will be picked up automatically from the database.

``bit_numbering_inverted`` - (optional, defaults to the value of the message
set) This flag controls the default :ref:`bit numbering <bit-numbering>` for the
messages contained in this mapping. Messages in the mapping can override the bit
numbering by explicitly specifying their own value for this flag.

``enabled`` - (optional, true by default) Enable or disable all processing of
the CAN messages in a mapping. By default, a mapping is enabled; if this flag is
false, all CAN message and signals from the mapping will be excluded from the
generated source code.

Database-backed Mappings
--------------------------

If you use Vector DBC files to store your "gold standard" CAN signal
definitions, you can save some effort by using the static CAN messages
definition from the database instead of repeating it in JSON.

In the database ``mapping`` file referred to earlier, you only need to define
(at minimum) the generic name for each signal in the message.

The code generation script merges your JSON mapping with an XML version of the
database. It pulls the necessary details of the messages from the database (bit
position, bit size, offset, etc.), saving you from defining the tedious and
error-prone parts in multiple places.

Extra Sources
=============

The ``extra_sources`` key is an optional list of C++ source files that should be
injected into the generated ``signals.cpp`` file. These may include value
handlers, message handlers, initializers or custom loopers.

Commands
========

The ``commands`` field is a mapping of arbitrary command names to functions that
should be called to run arbitrary code in the VI on-demand (e.g. sending
multiple CAN signals at once). The value of this attribute is a list of objects
with these attributes:

``name`` - The name of the command to be recognized on the OpenXC translated
interface.

``enabled`` - (optional, true by default) Enable or disable all processing of a
command. By default, a command is enabled. If this flag is false, the command
will be excluded from the generated source code.

``handler`` - The name of a custom command handler function (that matches the
``CommandHandler`` function prototype from ``canutil.h``) that should
be called when the named command arrives over the translated VI interface (e.g.
USB or Bluetooth).

.. code-block:: c

    bool (*CommandHandler)(const char* name, cJSON* value, cJSON* event,
            CanSignal* signals, int signalCount);

Any message received from the USB host with that given command name will be
passed to your handler. This is useful for situations where there isn't a 1 to
1 mapping between OpenXC command and CAN signal, e.g. if the left and right turn
signal are split into two signals instead of the 1 state-based signal used by
OpenXC. You can use the ``sendCanSignal`` function in ``canwrite.h`` to do the
actual data sending on the CAN bus.

.. _bit-numbering:

Bit Numbering
=============

Because of different software tools and conventions in the industry, there are
multiple ways to refer to bits within a CAN message. This doesn't change the
actual data representation (like a different *byte* order would) but it changes
how you refer to different bit positions for CAN signals.

The vehicle interface C++ source assumes the number of the highest order bit of
a 64-bit CAN message is 0, and the numbering continuous left to right:

.. code-block:: none

   Hex:         0x83                     46
   Binary:      10000011              01000110
   Bit pos:   0 1 2 3 4 5 6 7   8 9 10 11 12 13 14 15 ...etc.

The tool used at Ford to document CAN messages (Vector DBC files) uses an
"inverted" numbering by default. In each byte of a CAN message, they start
counting bits from the *rightmost bit*, e.g.:

.. code-block:: none

   Hex:         0x83                     46
   Binary:      10000011              01000110
   Bit pos:   7 6 5 4 3 2 1 0   15 14 13 12 11 10 9 8 ...etc.

When building ``CanSignal`` structs manually, you must use the normal,
non-inverted bit numbering.

When using JSON mapping format and the code generation tools, you can control
the bit numbering with the ``bit_numbering_inverted`` flag. By default it
assumes inverted bit ordering (since the most common use case for the mappings
up until now is the pull data from DBC files).
