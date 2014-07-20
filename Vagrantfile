# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # For a complete reference, please see the online documentation at
  # vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "ubuntu/trusty64"

  config.vm.provision "shell", privileged: false, keep_color: true do |s|
    # To avoid trying to create hardlinks in the /vagrant share, copy the
    # scripts locally and install the dependencies from there.
    s.inline = "VAGRANT=1 /vagrant/script/bootstrap.sh"
  end
end
