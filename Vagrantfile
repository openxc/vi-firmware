# -*- coding: utf-8 -*-
# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

def total_cpus
  require 'etc'
  Etc.nprocessors
end

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # For a complete reference, please see the online documentation at
  # vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "bento/ubuntu-18.04"
  config.vm.box_version = "201912.03.0"
  config.vm.provider :virtualbox do |v|
    if total_cpus > 1
      v.cpus = total_cpus - 1
	else
	  v.cpus = 1
	end
  end
  
  # Check for proxy enviroment variable and set it
  if ENV['HTTP_PROXY'] || ENV['HTTPS_PROXY']
    if Vagrant.has_plugin?("vagrant-proxyconf")
      if ENV['HTTP_PROXY']
        config.proxy.http = ENV['HTTP_PROXY']
        config.apt_proxy.http = ENV['HTTP_PROXY']
      end
      if ENV['HTTPS_PROXY']
        config.proxy.https = ENV['HTTPS_PROXY']
        config.apt_proxy.https = ENV['HTTP_PROXY']
      end
      if ENV['NO_PROXY'] 
        config.proxy.no_proxy = ENV['NO_PROXY']
      end
    else
      abort("ERROR, vagrant-proxyconf not installed run ‘vagrant plugin install vagrant-proxyconf’ to install it") 
    end
  end

  config.vm.box_download_insecure = true
  config.vm.provision "shell", privileged: false, keep_color: true do |s|
    s.inline = "ln -fs /vagrant vi-firmware;"
    s.inline += "VAGRANT=1 vi-firmware/script/bootstrap.sh"

  end
end
