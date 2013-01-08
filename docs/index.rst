OpenXC CAN Translator
=================================================

About
=====

.. include:: ../README.rst

Pre-built Binary
----------------

If you've downloaded a pre-built binary for a specific vehicle, see the [firmware
section](http://openxcplatform.com/vehicle-interface/firmware.html) of the
[OpenXC website](http://openxcplatform.com) for instructions on how to flash
your CAN translator. Most users do not need to set up the full development
described in these docs.

Output Specification
---------------------

See the [output
format](http://openxcplatform.com/vehicle-interface/output-format.html) section
of the [OpenXC][] website for details on the output format.

Testing
-----------

The [OpenXC Python library](https://github.com/openxc/openxc-python), in
particular the `openxc-dashboard` tool, is useful for testing the CAN translator
with a regular computer, to verify the data received from a vehicle before
introducing an Android device. Documentation for this tool (and the list of
required dependencies) is available on the OpenXC [vehicle interface
testing](http://openxcplatform.com/vehicle-interface/testing.html) page.

Traces
-------

You can record a trace of JSON messages from the CAN reader for use in testing.
First, install the [OpenXC Python library](python-lib). Then attach the CAN
translator to your computer via USB and use the `openxc-dump` program to print
all raw JSON messages to stdout. Redirect this to a file, and you've got your
trace. This can be used directly by the openxc-android library, for example.

    $ openxc-dump > vehicle-data.trace

[python-lib]: https://github.com/openxc/openxc-python

Output Specification
--------------------

See the [output
format](http://openxcplatform.com/vehicle-interface/output-format.html) section
of the [OpenXC][] website for details on the output format.

.. toctree::
   :maxdepth: 2
    :glob:

Contributing
==============

Please see our [Contibution Guide](https://github.com/openxc/cantranslator/blob/master/CONTRIBUTING.mkd)
and additional [developer documentation](dev-docs).

Mailing list
------------

For discussions about the usage, development, and future of OpenXC, please join
the `OpenXC mailing list`_.

.. _`OpenXC mailing list`: http://groups.google.com/group/openxc

Bug tracker
------------

If you have any suggestions, bug reports or annoyances please report them
to our issue tracker at http://github.com/openxc/openxc-python/issues/
