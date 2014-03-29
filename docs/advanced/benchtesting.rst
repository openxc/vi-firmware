=====================
CAN Bench Testing
=====================

Normally, the CAN controllers are initialized in a "listen only" mode. They are
configured as writable only if using raw CAN passthrough with a CanBus that is
marked ``writable`` or if a CanSignal is ``writable``.

This works fine in a vehicle, but when testing on a bench with a simulated CAN
network (e.g. another VI sending CAN messages directly to your VI under test),
there's a problem - every CAN message must be acknowledged by the controller,
and in "listen only" mode it does not send these ACKs. With nobody ACKing on the
bus, the messages never propagate up from the network layer to the VI firmware.

When compiling the VI firmware to use to receive data on a bench test CAN bus,
use the ``DEFAULT_CAN_ACK_STATUS`` flag to make sure CAN messages are ACked:

.. code-block:: sh

    $ make clean
    $ DEFAULT_CAN_ACK_STATUS=1 make

The CAN controllers will also be write-enabled if either CAN bus is configured
to accept raw CAN or diagnostic message requests, or a writable translated
signal is included.

Simulating a CAN Network
========================

Speaking of CAN bus bench testing, it's possible to create a 2-node CAN network
on your desk with 2 VIs to test your firmware. This is especially handy for
prototyping new translated signals, since you can record a raw CAN bus trace
from a vehicle and do all of your work inside where it's warm and comfortable.

Requirements
````````````

- 2 VIs

- A female-female OBD-II cable, or whatever combination of cable ends you need
  to connect one VI directly to another

- 120 ohm resistors across the H/L CAN wires in that cable or...

  - If one VI is a chipKIT, there are jumpers you can flip to enable 120 ohm
    resistors on each CAN controller (only one of the VIs needs the resistors)

  - If one VI is a Ford prototype, that has the termination resistors enabled by
    default on version 1.0. In an updated version that may change, as this is
    actually a mistake - they should have been only available if you short a
    solder jumper.

We recommend building your own cable with `female OBD-II connectors from Molex
<http://www.digikey.com/product-search/en?mpart=0511151601&vendor=900>`_ (with
the matching `crimp connectors
<http://www.digikey.com/product-search/en?mpart=0504208000&vendor=900>`_ and a
`crimper
<http://www.digikey.com/catalog/en/partgroup/premiumgrade-obd-ii-50420/22595>`_
wired together with 120ohm resistors crimped directly into one end of the cable
(some `pictures of a wired cable
<https://plus.google.com/photos/108408483770573977605/albums/5931052847037606033?authkey=CMeO7oewgMP2bA>`_).
We also recommend including the 12v and a ground wire, and shaving off a little
ring of insulation on each at one end of the connector so you can inject bus
power if needed. Make sure to offset the sections of shaved off insulation by a
half inch so they don't short.

Alternatively, there are OBD-II splitter cables available online that may also
work, but these have not been tested.

Preparing the Transmitter
=========================

1. Decide which of the VIs is the transmitter and which is the receiver. The
   receiver is most like your VI under test.
2. Follow the normal firmware build process (the CAN signals defined don't
   matter, just the bus speeds) but set the ``TRANSMITTER`` environment variable:
   ``TRANSMITTER=1 make``. This changes the USB product ID from 1 to 2,
   so you can address the transmitter VI independently from the receiver. TODO
   create a transmitter config file with raw writable bus and no power
   management.

Preparing the Receiver
=======================

Compile the VI firmware for the receiver as usual, but instead of just ``make``,
run ``DEFAULT_CAN_ACK_STATUS=1 make`` This configures the CAN controllers as write-enabled,
so that your VI under test can ACK the CAN messages. If nobody on the bus ACKs,
you will receive nothing. In a car there are usually many other things ACKing,
so we can be "listen only".

Sending Data on the Bus
========================

You need a pre-recorded raw CAN trace file from a vehicle. To create one,
compile the firmware in the passthrough mode (see the :doc:`low-level CAN docs
</advanced/lowlevel>`) and flash a VI, then use ``openxc-dump`` to receive the
raw messages - point that at a file on disk to create a trace.

With a raw CAN trace file named ``raw-can-trace.json``:

  .. code-block:: sh

    $ openxc-control writeraw --usb-product 2 -f raw-can-trace.json

That will send all of the message in the file, correctly spaced according to the
timestamps. If you want to send continuously, you can loop the file like this:

  .. code-block:: sh

    $ watch -n 0 openxc-control writeraw --usb-product 2 -f raw-can-trace.json

Receiving Data
==============

Receive data as usual from the VI under test, as if it was in real car.
