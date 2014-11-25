=============================
Development Environment Setup
=============================

Before you can compile, you need to set up our development environment. You only
need to do this once.

When you see ``$`` it means this is a shell command - run the command after the
``$`` but don't include the ``$``. The shell commands may also be prefixed with
a folder name, meaning it needs to be run from a particular location, e.g.
``foo/ $ ls`` means to run ``ls`` from the ``foo`` folder.

.. _git:

Install Git
============

You need to install Git to retrieve the source code repository for the
``vi-firmware``.

Windows
^^^^^^^

Install `GitHub for Windows <https://windows.github.com/>`_, which includes a
GUI for cloning Git repositories hosted at GitHub as well as a Bash terminal
for using Git from the command line.

Mac OS X
^^^^^^^^

Install `GitHub for Mac <https://mac.github.com/>`_, which includes a
GUI for cloning Git repositories hosted at GitHub and also installs the
``git`` command line tools.

Linux
^^^^^

If you're using Linux, you hopefully know what you're doing and can install
Git for your distro of choice. A couple of examples:

Ubuntu:

.. code-block:: sh

   $ sudo apt-get install git

Arch Linux:

.. code-block:: sh

   $ [sudo] pacman -S git

Clone the vi-firmware Repository
================================

On Windows or Mac and you installed the GitHub app, open the `vi-firmware
<https://github.com/openxc/vi-firmware>`_ repository in you browser and click
the "Clone in Desktop" button.

If you are using ``git`` from the command line, clone it like so:

.. code-block:: sh

   $ git clone https://github.com/openxc/vi-firmware

Install Vagrant (Recommended for All Platforms)
===============================================

On all platforms, we recommend using the included `Vagrant
<http://www.vagrantup.com>`_ configuration to get compiling as quickly as
possible. Vagrant is a tool that helps developers create and use identical
development environments. We include a ``Vagrantfile`` in the repository that
Vagrant uses to create a small Linux virtual machine with all of the vi-firmware
dependencies installed. Once installed, you can open a shell in this VM just by
running ``vagrant ssh``. The ``vi-firmware`` on the host OS (e.g. your Linux,
Windows or OS X machine) is shared to the Vagrant VM at the path ``/vagrant``.
You can use whatever text editor or IDE you prefer in your native OS, and simply
do the compilation from within the VM.

#. Install `VirtualBox <https://www.virtualbox.org/>`_.
#. If using Windows 7 (not required in Windows 8), add the VirtualBox tools to
   your PATH: ``PATH=%PATH%;c:\Program Files\Oracle\VirtualBox``. If you aren't
   sure how to edit your ``PATH``, see `this guide for all versions of Windows
   <https://www.java.com/en/download/help/path.xml>`_. Log out and back in for
   the change to take effect.
#. `Install Vagrant <http://docs.vagrantup.com/v2/installation/index.html>`_.
#. Navigate to the ``vi-firmware`` repository in the GitHub app, click the gear
   icon in the top right corner and select "Open in Git Shell".
#. In the shell, run ``vagrant up`` to initialize the Vagrant VM - this may take
   up to 10 minutes as it downloads and installs a number of dependencies in the
   VM.
#. If the initialization completes with no errors, run ``vagrant ssh`` to open a
   shell in the VM.
#. The ``vi-firmware`` directly is shared with the VM in the default home
   directory. Move into that directory and compile away:

.. code-block:: sh

   $ cd vi-firmware
   ~/vi-firmware $ fab reference build

You can edit the source code from your native OS and re-run ``fab reference
build`` from the Vagrant shell to compile (to compile for the reference VI, for
example). Run ``vagrant suspend`` to suspend the VM and ``vagrant up`` again to
restore it - it will be much faster after the first run.

Native Development
==================

Don't want to use Vagrant? There are varying levels of support for compiling in
your native OS - see the :doc:`native development environment docs
</compile/native-development>`.
