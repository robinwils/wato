# -*- mode: ruby -*-
# vi: set ft=ruby :

# Base box: https://github.com/akrabat/packer-templates

Vagrant.configure("2") do |config|
  config.vm.box = "peru/windows-10-enterprise-x64-eval"
  config.vm.box_version = "20240201.01"
  config.vm.guest = :windows
  config.vm.boot_timeout = 600

  config.vm.hostname = "WinDev"

  config.vm.communicator = "winrm"
  config.vm.synced_folder ".", "/vagrant", type: "rsync",
    rsync__exclude: ".git/"
  config.vm.provider :libvirt do |libvirt|
    # Display the VirtualBox GUI when booting the machine
    # vb.gui = true
    # Customize the amount of memory on the VM:
    libvirt.host = "WinDev"
    libvirt.memory = "4096"
    libvirt.cpus = 4
    libvirt.uri = 'qemu+unix:///system'
    libvirt.driver = 'kvm'
  end
  config.vm.provision "shell", privileged: "true", inline: <<-'POWERSHELL'
    Set-TimeZone "Coordinated Universal Time"

    # Install Boxstarter
    Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://boxstarter.org/bootstrapper.ps1')); Get-Boxstarter -Force
    
    choco install -y visualstudio2022buildtools --params "--includeRecommended --includeOptional --add Microsoft.VisualStudio.Component.VC.14.38.17.8.x86.x64 --passive --locale en-US --installchanneluri https://aka.ms/vs/17/release.ltsc.17.8/channel"
  POWERSHELL
end
