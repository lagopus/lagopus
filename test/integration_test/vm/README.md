<!-- -*- mode: markdown -*- -->

How to build the environment for KVM.
======================================
## Recommended
* Host
    * Ubuntu 14.04
        * Vagrant 1.7.4
            * vagrant-libvirt 0.0.32
            * vagrant-mutate 1.0.3
            * vagrant-reload 0.0.1
    * qemu-kvm 2.0.0 (use KVM)
    * VirtualBox 5.0.10 (use VirtualBox)
* Gest
    * Ubuntu 14.04

How to build the environment.
---------------------------
## 1. Install packages.
  * Use KVM.
    Install packages for KVM, Ruby, ..,etc.
    You execute the following command:

        % sudo apt-get update
        % sudo apt-get install \
           qemu-kvm libvirt-bin ubuntu-vm-builder bridge-utils \
           libxslt-dev libxml2-dev libvirt-dev dpkg zlib1g-dev \
           ruby-dev rubygems
        % sudo gem install nokogiri

   Detailed Installation of KVM see below.
   cf.) https://help.ubuntu.com/community/KVM/Installation

  * Use VirtualBox.
    Install packages for VirtualBox.
    Detailed Installation of  see below.
    cf.) https://www.virtualbox.org/manual/UserManual.html

## 2. Install Vagrant.
   Install Vagrant packages.
   You execute the following command:

    % wget https://dl.bintray.com/mitchellh/vagrant/vagrant_1.7.4_x86_64.deb
    % sudo dpkg -i vagrant_1.7.4_x86_64.deb

    # plugins.
    % vagrant plugin install vagrant-libvirt --plugin-version 0.0.32
    % vagrant plugin install vagrant-mutate --plugin-version 1.0.3
    % vagrant plugin install vagrant-reload --plugin-version 0.0.1

## 3. Add box image.
   Add Vagrant box.
   You execute the following command:

    e.g.)
    % vagrant box add lagopus_ubuntu_14.04s \
       https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-amd64-vagrant-disk1.box

    # use KVM.
    % vagrant mutate lagopus_ubuntu_14.04s libvirt

## 4. Edit `conf/vagrant_conf.yml`, `conf/ansible_conf.yml` file.

    % vi conf/vagrant_conf.yml
    % vi conf/ansible_conf.yml

## 5. Start Vagrant.

    # use VirtualBox.
    vagrant up

    # use KVM.
    % vagrant up --provider=libvirt

    # login
    % vagrant ssh [<HOST_NAME>]


  Detailed command see below.
  cf.) https://docs.vagrantup.com/v2/cli/index.html

conf/vagrant_conf.yml
---------------------------
## Configuration items
|field1|field2|field3|description|
|:--|:--|:--|:--|
|hosts|||Host configurations.|
||name|||
||cpus|||
||memory|||
||interfaces|||
|||IP_ADDR|IP addr.|
|system||||
||box_name|||
||vagrant_dir|||
|ssh||||
||forward_agent|||
|libvirt||||
||storage_pool|||
|virtualbox||||
||intnet_name|||

## Sample

    # hosts
    hosts:
      - name: lagopus01
        cpus: 4
        memory: 4096
        interfaces:
          - 192.168.100.100
          - 192.168.101.100
          - 192.168.102.100
          - 192.168.103.100
      - name: lagopus02
        cpus: 4
        memory: 4096
        interfaces:
          - 192.168.100.101
          - 192.168.101.101
          - 192.168.102.101
          - 192.168.103.101

    system:
      box_name: lagopus_ubuntu_14.04s
      vagrant_dir: /vagrant

    ssh:
      forward_agent: false

    libvirt:
      storage_pool: default

    virtualbox:
      intnet_name: intnet

conf/ansible_conf.yml
---------------------------
## Configuration items
|field1|field2|description|
|:--|:--|:--|
|system|||
||shell_dir||
||work_dir|Working directory for lagopus, DPDK.|
|dpdk|||
||is_used|Use DPDK.|
|dpdk_env||Environment variables for DPDK.|
||RTE_SDK||
|lagopus|||
||git|Git Repositorie URI (https://..., ssh://USER_NAME@HOST:PORT/..., ..etc) |
||dir|Clone directory.|
||is_used_git_ssh|use ssh.|
|proxy_env|||
||http_proxy||
||https_proxy||

## Sample

    # configuration for ansible.
    system:
      shell_dir: "{{ vagrant.system.vagrant_dir }}/lib/shell"
      work_dir: "{{ ansible_env.HOME }}/work"

    dpdk:
      is_used: true
      version: dpdk-2.1.0

    dpdk_env:
      RTE_SDK: "{{ system.work_dir }}/{{ dpdk.version }}"
      RTE_TARGET: "x86_64-native-linuxapp-gcc"

    lagopus:
      git: https://github.com/lagopus/lagopus.git
      dir: "{{ system.work_dir }}/lagopus"
      is_used_git_ssh: false

    proxy_env:
      http_proxy: http://proxy.foo.com:80
      https_proxy: http://proxy.foo.com:80

How to use SSH agent forwarding for git.
---------------------------
## 1. Add your SSH key in HOST.

    ssh-add ~/.ssh/<KEY>

## 2. Edit `conf/vagrant_conf.yml`, `conf/ansible_conf.yml`.

    diff --git a/test/integration_test/vm/conf/ansible_conf.yml b/test/integration_test/vm/conf/ansible_conf.yml
    --- a/test/integration_test/vm/conf/ansible_conf.yml
    +++ b/test/integration_test/vm/conf/ansible_conf.yml
     lagopus:
    -  git: https://github.com/lagopus/lagopus.git
    +  git: ssh://USER_NAME@HOST:PORT/REPOS  # Your repository.

    -  is_used_git_ssh: false
    +  is_used_git_ssh: yes

    diff --git a/test/integration_test/vm/conf/vagrant_conf.yml b/test/integration_test/vm/conf/vagrant_conf.yml
    --- a/test/integration_test/vm/conf/vagrant_conf.yml
    +++ b/test/integration_test/vm/conf/vagrant_conf.yml
     ssh:
    -  forward_agent: false
    +  forward_agent: yes

## 3. Start Vagrant.

    % vagrant up [OPTS]
