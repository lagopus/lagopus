Quick Start for FreeBSD
==========================

Software Installation
--------------------------

### Lagopus vswitch installation

Install necessary packages

```
	$ sudo pkg install git gmake gmp bash subversion
```

Compile vswitch

* DPDK version

RESTRICTION: So far, we need little hack to build lagopus with DPDK.

```
        $ svn checkout https://svn.freebsd.org/base/release/11.1.0/sys
	$ cd lagopus
        $ mkdir temp; echo clean: > temp/Makefile
	$ env RTE_KERNELDIR=$PWD/temp SYSDIR=$HOME/sys CC=clang CONFIG_SHELL=/usr/local/bin/bash ./configure
	$ gmake
```

* raw socket version

```
	$ cd lagopus
	$ env CONFIG_SHELL=/usr/local/bin/bash ./configure --disable-dpdk
	$ gmake
```

