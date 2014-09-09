===================================
Compiling the Default Configuration
===================================

Going beyond emulated data, you can start using your VI with a real vehicle by
using the default configuration. default configuration. This build is not
configured to read any particular CAN signals or messages, but it allow you to
send On-Board Diagnostic (OBD-II) requests and raw CAN messages for
experimentation.

Again, assuming you've :doc:`set up your development environment
</getting-started/development-environment>` and you have a `reference VI from
Ford <http://vi.openxcplatform.com>`_, compile the default- the default build
for the reference VI.

If you are using Vagrant, ``cd`` into the ``/vagrant`` directory (which is
actually a pointer to the ``vi-firmware`` directory on your host computer) and
run ``fab reference build``:

.. code-block:: sh

    $ cd /vagrant
    /vagrant $ fab reference build
    Compiling for FORDBOARD...
    ...lots of output...
    Compiled successfully for FORDBOARD running under a bootloader.

If you are doing native development, it's the same process but ``cd`` into
wherever you put the ``vi-firmware`` directory instead of ``/vagrant``.

Just as with the emulated data build, there will be a lot more output when you
run this but it should end with ``Compiled successfully...``. If you got an
error, try and follow what it suggests, then look at the troubleshooting
section, and finally ask for help on the `Google Group
</overview/discuss.html>`_.

Re-flash your VI (go back to the section on :doc:`compiling-emulator` if
you forgot how to do that), and try the ``openxc-version`` command again to make
sure it's running your new version.

You can use the ``openxc-diag`` tool (also from the OpenXC Python library) to
send a simple OBD-II request for the engine speed (RPM) to your car. Plug the VI
into your car, then attach via USB and run:

.. code-block:: sh

    $ openxc-diag --id 0x7df --mode 1 --pid 0xc
    {"success": true, "bus": 1, "id": 2015, "mode": 1, "pid": 12, "payload": "0x0"}
