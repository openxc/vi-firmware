===============================
Developing in a Native Host OS
===============================


On all platforms, we recommend using the included `Vagrant
<http://www.vagrantup.com>`_ configuration to get compiling as quickly as
possible with a Linux virtual machine (VM).

If you are an advanced developer and you want to compile in your host OS, we may
have support for it. Beware tha there are number of dependencies and we've found
it finicky to replicate on the huge variety of possible configurations. Unless
you have a compelling reason not to, we strongly recommend using :doc:`Vagrant
</getting-started/development-environment>`.

The dependencies can be installed using the included ``bootstrap.sh`` shell
script. The script officially supports and is tested regularly in:

* Ubuntu Linux 13.04 - 14.04
* Arch Linux

and it has unofficial support (i.e. it's been tested, but not regularly) for:

* Mac OS X Mavericks
* Cygwin in Windows. We've found Cygwin can be a huge pain to get consistently
  set up, so we strongly discourage you from using it unless you are an expert
  in debugging Cygwin-specific problems.

Even if you already use Linux, we still recommend using Vagrant to set up a VM.
The huge variety of Linux installations means that it's often much more pleasant
to use the Vagrant VM which has a stable, repeatable setup.

Pre-requisites
==============

Follow the steps in :doc:`/getting-started/development-environment` to install Git and clone
the ``vi-firmware`` repository.

Cygwin Pre-Setup
================

Compiling natively in Windows (e.g. at a Windows Command Prompt) is not
supported. Cygwin (32-bit) is unofficially supported, but is no longer
recommended. You'll be much happier using Vagrant, promise!

If you still wish to use Cygwin, download 32-bit version of `Cygwin
<http://www.cygwin.com>`__ (even if you're on 64-bit Windows) and run the
installer - during the installation process, select these packages:

.. code-block:: sh

    make, gcc-core, patchutils, unzip, python, check, curl, libsasl2, python-setuptools

After it's installed, open a new Cygwin terminal and configure it to ignore
Windows-style line endings in scripts by running this command:

.. code-block:: sh

    $ set -o igncr && export SHELLOPTS

Bootstrap
==========

Run the ``bootstrap.sh`` script:

.. code-block:: sh

   $ cd vi-firmware
   vi-firmware/ $ script/bootstrap.sh

If there were no errors, you are ready to compile. If there are errors, try to
follow the recommendations in the error messages. You may need to :doc:`manually
install the dependencies </compile/dependencies>` if your environment is not in a
predictable state. If you're stuck, you might seriously consider using
:doc:`Vagrant </getting-started/development-environment>`!
