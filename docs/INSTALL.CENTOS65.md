Lagopus installation manual on CentOS 6.5
=========================================

Required packages installation for Lagopus
------------------------------
Install required development tools and libraries:

	% sudo yum install unzip patch gcc kernel-devel python-devel \
	byacc expat-devel gmp-devel openssl-devel libpcap-devel flex \
	pciutils

Intel DPDK setup
------------------------------
Please refer "QUICKSTART.md".

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
