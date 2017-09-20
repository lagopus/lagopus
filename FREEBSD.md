Quick Start for FreeBSD
==========================

Software Installation
--------------------------

### Lagopus vswitch installation

Install necessary packages

```
	$ sudo pkg install git gmake gmp gcc bash subversion
        $ svn checkout https://svn.freebsd.org/base/release/11.1.0/sys
```

Compile vswitch

* DPDK version

```
	$ cd lagopus
	$ env SYSDIR=$HOME/sys CONFIG_SHELL=/usr/local/bin/bash ./configure
	$ gmake
```

* raw socket version

```
	$ cd lagopus
	$ env SYSDIR=$HOME/sys CONFIG_SHELL=/usr/local/bin/bash ./configure --disable-dpdk
	$ gmake
```

