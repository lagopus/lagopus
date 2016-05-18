Lagopus Software Switch
==========================

Lagopus software switch is a yet another OpenFlow 1.3 software switch implementation.
Lagopus software switch is designed to leverage multi-core CPUs for high-performance packet processing and fowarding with DPDK.
Many network protocol formats are supported, such as Ethernet, VLAN, QinQ, MAC-in-MAC, MPLS and PBB. In addition, tunnel protocol processing is supported for overlay-type networking with GRE, VxLAN and GTP.

How to use Lagopus vswitch
==========================

- For a first trial, you may follow the [quick start instructions](QUICKSTART.md).
- [User guide](http://lagopus.github.io/lagopus-book/en/html/)

Supported hardware
==========================
Lagopus can run on Intel x86 servers and virtual machine.

- CPU
  - Intel Xeon, Core, Atom
- NIC
  - standard NICs with raw-socket
  - [DPDK-enabled NICs](http://dpdk.org/doc/nics)
- Memory: 2GB or more

Supported distribution
===============================
- Linux
  - Ubuntu 14.04, Ubuntu 16.04
  - CentOS 7
- FreeBSD
- NetBSD

Support
==========================
Lagopus Official site is <https://lagopus.github.io/>.

- Mailing list.
    - [Developement ML](https://lists.sourceforge.net/lists/listinfo/lagopus-devel)
    - [Japanese discussion ML](https://lists.sourceforge.net/lists/listinfo/lagopus-jp-discuss)
- Slack
    - Register account in [slackin page](https://lagopus-project-slack.herokuapp.com/)

Development
==========================

Your contribution are very welcome, submit your patch with
[Github Pull requests](https://github.com/lagopus/lagopus/pulls).
Or if you find any bug, let us know with [Github Issues](https://github.com/lagopus/lagopus/issues).

License
==========================
All of the code is freely available under the Apache 2.0 license.
