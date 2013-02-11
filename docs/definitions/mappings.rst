==========================
JSON Mapping Format
==========================

The code generation script (to generate ``signals.cpp``) requires a JSON file of
a specific format as input. The input format is a JSON object like the one found
in `sample.json
<https://github.com/openxc/cantranslator/blob/master/src/signals.json.example>`_.

JSON Format
============

The root level JSON object maps CAN bus addresses to CAN bus objects,  CAN
message IDs to CAN message objects in each bus, and CAN signal name to signal
object within each message.

CAN Bus
-------

The object key for a CAN bus is a hex address that identifies which CAN
controller on the microcontroller is attached to the bus. The platforms we are
using now only have 2 CAN controllers, but by convention they are identified
with ``0x101`` and ``0x102`` - these are the only acceptable bus addresses.

``speed`` - The CAN bus speed in Kbps.

``messages`` - A mapping of CAN message objects that are on this bus,
the key being the message ID in hex as a string (e.g. ``0x90``).

``commands`` - A mapping of CAN command objects that should be sent on
this bus that should be sent on this bus. The key is the name that will
be used over the OpenXC interface.

Command
-------

The attributes of a ``command`` object are:

``handler`` - The name of a custom command handler function that should
be called with the data when the named command arrives over
USB/Bluetooth/etc.

Message
-------

The attributes of a ``message`` object are:

``name`` - The name of the CAN message. Optional - just used to be able
to reference the original documentation from the mappings file.

``handler`` - The name of a function that will be compiled with the
sketch and should be applied to the entire raw message value. No other
operations are performed on the data if this type of handler is used.
Optional - see the "Custom Handlers" section for more.

``signals`` - A list of CAN signal objects that are in this message,
with the official name of the signal as the key. If merging with
automatically generated JSON from another database, this value must
match exactly - otherwise, it's an arbitrary name.

Signal
-------

The attributes of a ``signal`` object within a ``message`` are:

``generic_name`` - The name of the associated generic signal name (from
the OpenXC specification) that this should be translated to. Optional -
if not specified, the signal is read and stored in memory, but not sent
to the output bus. This is handy for combining the value of multiple
signals into a composite measurement such as steering wheel angle with
its sign.

``bit_position`` - The staring bit position of this signal within the
message.

``bit_size`` - The width in bits of the signal.

``factor`` - The signal value is multiplied by this if set. Optional.

``offset`` - This is added to the signal value if set. Optional.

``value_handler`` - The return type and name of a function that will be
compiled with the sketch and should be applied to the signal's value
after the normal translation. Optional - see the "Custom Handlers"
section for more.

``ignore`` - Setting this to ``true`` on a signal will silence output of
that signal. The translator will not monitor the signal nor store any of
its values. This is useful if you are using a custom handler for an
entire message, want to silence the normal output of the signals it
handles, and you don't need the translator to keep track of the values
of any of the signals separately. If you need to use the previously
stored values of any of the signals, you can use the ``ignoreHandler``
as a value handler for the signal.

``states`` - For state values, this is a mapping between the desired
descriptive enum states (e.g. ``off``) and a list of the corresponding
raw state values from the CAN bus (usually an integer). The raw values
are specified as a list to accommodate multiple raw states being
coalesced into a single final state (e.g. key off and key removed both
mapping to just "off").

``send_frequency`` - Some CAN signals are sent at a very high frequency,
likely more often than will ever be useful to an application. This
attribute defaults to ``1`` meaning that ``1/1`` (i.e. 100%) of the
values for this signal will be processed and sent over USB. Increasing
the value will reduce the number of messages that are sent - a value of
``10`` means that only ``1/10`` messages (i.e. every 10th message) is
processed. You don't want to combine this attribute with ``send_same``
or else you risk missing a status change message if wasn't one of the
messages the translator decided to let through.

``send_same`` - By default, all signals are process and sent over USB
every time they are received on the CAN bus. By setting this to
``false``, you can force a signal to be sent only if the value has
actually changed. This works best with boolean and state based signals.

``writable`` - The only signals read through the ``OUT`` channel of the
USB device (i.e. from the host device back to the CAN translator) that
are actually encoded and written back to the CAN bus are those marked
with this flag true. By default, the value will be interpreted as a
floating point number.

``write_handler`` - If the signal is writable and is not a plain
floating point number (i.e. it is a boolean or state value), you can
specify a custom function here to encode the value for a CAN messages.
This is only necessary for boolean types at the moment - if your signal
has states defined, we assume you need to encode a string state value
back to its original numerical value.

Device to Vehicle Commands
===========================

Optionally, you can specify completely custom handler functions to
process incoming OpenXC messages from the USB host. In the ``commands``
section of the JSON object, you can specify the generic name of the
OpenXC command and an associated function that matches the
``CommandHandler`` function prototype (from ``canutil.h``):

.. code-block:: c

    bool (*CommandHandler)(const char* name, cJSON* value, cJSON* event,
            CanSignal* signals, int signalCount);

Any message received from the USB host with that name will be passed to
your handler - this is useful for situations where there isn't a 1 to 1
mapping between OpenXC command and CAN signal, e.g. if the left and
right turn signal are split into two signals instead of the 1
state-based signal used by OpenXC. You can use the ``sendCanSignal``
function in ``canwrite.h`` to do the actual data sending on the CAN bus.

Message Handlers
=======================

The default handler for each signal is a simple passthrough, translating
the signal's ID to an abstracted name (e.g. ``SteeringWheelAngle``) and
its value from engineering units to something more usable. Some signals
require additional processing that you may wish to do within the
translator and not on the host device. Other signals may need to be
combined to make a composite signal that's more meaningful to
developers.

An good example is steering wheel angle. For an app developer to get a
value that ranges from e.g. -350 to +350, we need to combine two
different signals - the angle and the sign. If you want to make this
combination happen inside the translator, you can use a custom handler.

You may also need a custom handler to return a value of a type other
than float. A handler is provided for dealing with boolean values, the
``booleanHandler`` - if you specify that as your signal's
``value_handler`` the resulting JSON will contain ``true`` for 1.0 and
``false`` for 0.0. If you want to translate integer state values to
string names (for parsing as an enum, for example) you will need to
write a value handler that returns a ``char*``.

There are two levels of custom handlers:

-  Message handlers - use these for custom processing of the entire CAN
   message.
-  Value handlers - use these for making non-standard transformations to
   a signal value

For this example, we want to modify the value of ``SteeringWheelAngle``
by setting the sign positive or negative based on the value of the other
signal (``StrAnglSign``). Every time a CAN signal is received, the new
value is stored in memory. Our custom handler
``handleSteeringWheelAngle`` will use that to adjust the raw steering
wheel angle value. Modify the input JSON file to set the
``value_handler`` attribute for the steering wheel angle signal to
``handleSteeringWheelAngle``. If you're using ``generate_code.py``, the
handlers should be saved in ``src/handlers.h`` and ``src/handlers.cpp``:

``src/handlers.h``:

.. code-block:: c

    float handleSteeringWheelAngle(CanSignal* signal, CanSignal* signals,
            int signalCount, float value, bool* send);

``src/handlers.cpp``:

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
``signals`` is a an array of all signals, ``value`` is the raw value
from CAN and ``send`` is a flag to indicate if this should be sent over
USB.

The ``bool* send`` parameter is a pointer to a ``bool`` you can flip to
``false`` if this signal value need not be sent over USB. This can be
useful if you don't want to keep notifying the same status over and over
again, but only in the event of a change in value (you can use the
``lastValue`` field on the CanSignal object to determine if this is
true).

A known issue with this method is that there is no guarantee that the
last value of another signal arrived in the message or before/after the
value you're current modifying. For steering wheel angle, that's
probably OK - for other signals, not so much.

If you need greater precision, you can provide a custom handler for the
entire message to guarantee they arrived together. You can generate 0, 1
or many translated messages from one call to your handler function.

.. code-block:: c

    void handleSteeringWheelMessage(int messageId, uint64_t data,
            CanSignal* signals, int signalCount, Listener* listener);
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

Using a custom message handler will not stop individual messages for
each signal from being output. To silence them but still store their
values in ``signal->lastvalue`` as they come in, specify the special
``ignoreHandler`` as the ``value_handler`` for signals don't want to
double send. The reason we don't do this automatically is that not all
signals in a message are always handled by the same message handler.

Generating JSON from Vector CANoe Database
============================================

If you use Canoe to store your "gold standard" CAN signal definitions,
you may be able to use the included ``xml_to_json.py`` script to make
your JSON for you. First, export the Canoe .dbc file as XML - you can do
this with Vector CANdb++. Next, create a JSON file according to the format
defined above, but only define:

* CAN bus
* CAN messages
* Name of CAN signals within messages and their ``generic_name``
* Any custom handlers or commands

Assuming the data exported from Vector is in ``signals.xml`` and your minimal
mapping file is ``mapping.json``, run the script:

.. code-block:: sh

    $ ./xml_to_json.py signals.xml mapping.json signals.json

The script scans ``mapping.json`` to identify the CAN messages and
signals that you want to use from the XML file. It pulls the neccessary details
of the messages (bit position, bit size, offset, etc) and outputs the resulting
subset as JSON into the output file, ``signals.json``.

The resulting file together with ``mapping.json`` will work as input to the code
generation script.
