Lagopus installation manual on CentOS 6.5
=========================================

Required packages installation for Lagopus
------------------------------
Install required development tools and libraries:

	% sudo yum install unzip patch gcc kernel-devel python-devel \
	byacc expat-devel gmp-devel openssl-devel libpcap-devel flex \
	readline-devel pciutils

Intel DPDK setup
------------------------------
CentOS 6.5 needs a patch work for Intel DPDK 1.6.0.

1. Download Intel DPDK 1.6.0 and unzip archive
	$ wget https://downloads.sourceforge.net/project/lagopus/Intel-DPDK/DPDK-1.6.0-18.zip
	$ unzip DPDK-1.6.0-18.zip

Or

	$ git clone git://dpdk.org/dpdk
	$ cd dpdk
	$ git checkout origin/intel

2. Code patch

	$ cd DPDK-1.6.0-18 or cd dpdk
	$ patch -p1 < "<path to lagopus>/contrib/dpdk/dpdk-1.6.0-centos-6.5.patch"

Lagopus setup
------------------------------
Please refer "QUICKSTART.md".

Ryu setup
------------------------------
Install required development tools and libraries:

	% sudo yum update -y
	% sudo yum groupinstall -y 'development tools'
	% sudo yum install -y zlib-devel bzip2-devel openssl-devel \
	ncurses-devel sqlite-devel readline-devel tk-devel libxslt-devel


	% wget https://www.python.org/ftp/python/2.7.7/Python-2.7.7.tgz
	% tar zxvf Python-2.7.7.tgz
	% cd Python-2.7.7
	% ./configure —prefix=/usr/local
	% make
	% sudo make altinstall

Install setuptools:

	% wget --no-check-certificate \
	https://pypi.python.org/packages/source/s/setuptools/setuptools-1.4.2.tar.gz
	% tar zxvf setuptools-1.4.2.tar.gz
	% python2.7 setup.py install

Install greenlet due to broken dependency in case of CentOS 6.5:

	% pip2.7 install greenlet

Install Ryu:

	% pip2.7 install ryu
