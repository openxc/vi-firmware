OpenXC CAN Translator
=================================================

About
=====

.. include:: ../README.rst

Pre-built Binary
================

If you've downloaded a pre-built binary for a specific vehicle, see the
`firmware section`_ of the `OpenXC website`_ for instructions on how to flash
your CAN translator. Most users do not need to set up the full development
described in these docs.

.. _`OpenXC website`: http://openxcplatform.com
.. _`firmware section`: http://openxcplatform.com/vehicle-interface/firmware.html

Output Specification
====================

See the `output format`_ section of the `OpenXC website`_ for details.

Testing
=======

The `OpenXC Python library`_, in particular the `openxc-dashboard` tool, is
useful for testing the CAN translator with a regular computer, to verify the
data received from a vehicle before introducing an Android device. Documentation
for this tool (and the list of required dependencies) is available on the OpenXC
`vehicle interface testing`_ page.

.. _`vehicle interface testing`: http://openxcplatform.com/vehicle-interface/testing.html

.. toctree::
    :maxdepth: 2
    :glob:

    testing/*

Traces
=======

You can record a trace of JSON messages from the CAN reader for use in testing.
First, install the `OpenXC Python library`_. Then attach the CAN
translator to your computer via USB and use the `openxc-dump` program to print
all raw JSON messages to stdout. Redirect this to a file, and you've got your
trace. This can be used directly by the openxc-android library, for example.

    $ openxc-dump > vehicle-data.trace

Contributing
==============

Please see our `Contributing Guide`_.

.. _`Contributing Guide`: https://github.com/openxc/cantranslator/blob/master/CONTRIBUTING.mkd

Mailing list
------------

For discussions about the usage, development, and future of OpenXC, please join
the `OpenXC mailing list`_.

.. _`OpenXC mailing list`: http://groups.google.com/group/openxc

Bug tracker
------------

If you have any suggestions, bug reports or annoyances please report them
to our issue tracker at http://github.com/openxc/cantranslator/issues/

.. _`OpenXC Python library`: https://github.com/openxc/openxc-python
.. _`output format`: http://openxcplatform.com/vehicle-interface/output-format.html
