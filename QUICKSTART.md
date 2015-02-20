Quick Start
==========================

Software Installation
--------------------------

### Intel DPDK install
You should install Intel DPDK before Lagopus vswitch compilation.

Install necessary packages

	$ sudo apt-get install make coreutils gcc binutils

Install kernel headers

	$ sudo apt-get install linux-headers-`uname -r`

Get Intel DPDK 1.8.0 and extract Intel DPDK zip file

	$ wget http://dpdk.org/browse/dpdk/snapshot/dpdk-1.8.0.zip
	$ unzip dpdk-1.8.0.zip

Compile

	$ export RTE_SDK=<Absolute Path of Intel DPDK>
	$ export RTE_TARGET="x86_64-native-linuxapp-gcc"
	$ make config T=${RTE_TARGET}
	$ make install T=${RTE_TARGET}

### Lagopus vswitch installation

Install necessary packages

	$ sudo apt-get install build-essential libexpat-dev libgmp-dev libncurses-dev \
	  libssl-dev libpcap-dev byacc flex libreadline-dev \
	  python-dev python-pastedeploy python-paste python-twisted

Compile vswitch

In configure phase, you must speicify Intel DPDK option with abosolute path in which you installed

	$ cd lagopus
	$ ./configure --with-dpdk-dir=${RTE_SDK}
	$ make

Install lagopus package

	$ make install

### Install Ryu, an OpenFlow 1.3 controller (Optional)

Install necessary packages

	$ sudo apt-get install python-setuptools python-pip python-dev \
	libxml2-dev libxslt-dev

Using pip command is the easiest option:

	$ pip install ryu

Or install from the source code if you prefer:

	$ git clone git://github.com/osrg/ryu.git
	$ cd ryu; python ./setup.py install

Environmental Setup
--------------------------

### Setup DPDK environment

The kernel modules built are available in the build/kmod directory.

Loading the kernel module

	$ sudo modprobe uio
	$ sudo insmod ${RTE_SDK}/${RTE_TARGET}/kmod/igb_uio.ko
	$ sudo insmod ${RTE_SDK}/${RTE_TARGET}/kmod/rte_kni.ko


Make hugepages available to DPDK.
You can setup hugepage with two ways:

1. Manual confguration (Hugepage size: 2MB)

		$ sudo sh -c "echo 256 >  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages"
		$ sudo mkdir -p /mnt/huge
		$ sudo mount -t hugetlbfs nodev /mnt/huge

2. Script configuration

	Add hugepages option of the linux kernel to reserve 1024 pages of 2 MB.

		hugepages=1024

	Add the following line to /etc/fstab so that mount point can be made permanent across reboots

		nodev /mnt/huge hugetlbfs defaults 0 0


Check PCI ID to enable DPDK on 2nd, 3rd, and 4th NIC.

If NIC used for management (ex: ssh) was selected, you will lose connection.

	$ sudo ${RTE_SDK}/tools/dpdk_nic_bind.py --status

	Network devices using IGB_UIO driver
	====================================

	<none>

	Network devices using kernel driver
	===================================
	0000:02:01.0 '82545EM Gigabit Ethernet Controller (Copper)' if=eth0 drv=e1000 unused=igb_uio *Active*
	0000:02:02.0 '82545EM Gigabit Ethernet Controller (Copper)' if=eth1 drv=e1000 unused=igb_uio
	0000:02:03.0 '82545EM Gigabit Ethernet Controller (Copper)' if=eth2 drv=e1000 unused=igb_uio
	0000:02:04.0 '82545EM Gigabit Ethernet Controller (Copper)' if=eth3 drv=e1000 unused=igb_uio

	Other network devices
	=====================
	<none>

Unbound NICs from ixgbe driver.

	$ sudo ${RTE_SDK}/tools/dpdk_nic_bind.py -b igb_uio 0000:02:02.0 0000:02:03.0 0000:02:04.0

Check the current status of NICs whehter the 2nd, 3rd and 4th interface is registerd with igb_uio driver

	$ sudo ${RTE_SDK}/tools/dpdk_nic_bind.py --status

	Network devices using IGB_UIO driver
	====================================
	0000:02:02.0 '82545EM Gigabit Ethernet Controller (Copper)' drv=igb_uio unused=e1000
	0000:02:03.0 '82545EM Gigabit Ethernet Controller (Copper)' drv=igb_uio unused=e1000
	0000:02:04.0 '82545EM Gigabit Ethernet Controller (Copper)' drv=igb_uio unused=e1000

	Network devices using kernel driver
	===================================
	0000:02:01.0 '82545EM Gigabit Ethernet Controller (Copper)' if=eth0 drv=e1000 unused=igb_uio *Active*

	Other network devices
	=====================
	<none>

### Setup Lagopus configuration file
Sample Lagopus configuration is "lagopus/samples/lagopus.conf".

NOTE: *lagopus.conf* file must be located at the same directory of the executable of lagopus vswitch or
the following directory.

     % sudo cp lagopus/samples/lagopus.conf /etc/lagopus/
     % sudo vi /etc/lagopus/lagopus.conf

* Example:
  * Use DPDK port 0, 1, and 2 for Lagopus vswitch
	* NOTE: the ethX notation is not same of ethX which Linux kernel
      manages
  * OpenFlow controller is "127.0.0.1"


```
interface {
    ethernet {
        eth0;
        eth1;
        eth2;
    }
}
bridge-domains {
    br0 {
        port {
            eth0;
            eth1;
            eth2;
        }
        controller {
            127.0.0.1;
        }
    }
}
```

How to Run Lagopus vswitch
==========================

Examples of command line
--------------------------
The lagopus runtime is located at following path.
NOTE: Due to the current implemenation, *lagopus.conf* is located the
same directory of the executable of Lagopus vswitch.

```
$ cd ~/lagopus/src/cmds
```

### Simple run
* CPU core: 1 and 2
* Number of memory channel: 1
* NIC port: 0 and 1
* Packet processing workers are automatically assigned to CPU cores.
* Run in debug-mode

```
$ sudo lagopus -d -- -c3 -n1 -- -p3
```

### Run with explicit assignment to achieve maximum performance
* CPU core: 1 - 7
  * core # 2 for I/O RX
  * core # 3 for I/O TX
  * core # 4, # 5, #6, # 7 for packet processing
* Memory channel: 2
* NIC port: 0 and 1
* Run in debug-mode

```
$ sudo lagopus -d -- -cfe -n2 -- --rx '(0,0,2),(1,0,2)' --tx '(0,3),(1,3)' --w 4,5,6,7
```

### Run with packet-ordering in flow-level
* CPU core: 1 - 15
* Memory channel: 2
* NIC port: 0, 1, 2
* Worker assignment is automatic
* FIFOness option to ensure packet-ordering in flow-levle

```
$ sudo lagopus -- -cfffe -n2 -- -p7 --fifoness flow
```

Lagopus command option
--------------------------

### Global option
* _-d_ : (Optional)
  * Run in debug mode
* _--_ : (Optional)
  * run in daemon mode
* _-v|--version_ : (Optional)
  * Show version
* _-h|-?|--help_ : (Optional)
  * Show help
* _-l filename |--logfile filename_ : (Optional)
  * Specify a log/trace file path [default: syslog]
* _-p filename | --pidfile filename_ : (Optional)
  * Specify a pid file path [default: /var/run/lt-lagopus.pid]

### Intel DPDK Option
* _-c_ : (Mandatory)
  * Speicify which CPU cores are assigned to Lagopus with a hex
    bitmask of CPU port to use
  * Example: Only CPU core 0 and core 3 are assigned, then let bit0
    and bit3 on.
    0x1001 (binary) = 9 (Hexadecimal)

		```
		-c9
		```
* _-n_ : (Mandatory)
  * Specify number of memory channel which CPU supports
  * Range: 1,2,4
  * Example: a CPU supports dual memory channlel

		```
		-n2
		```
* _-m_ :(Optional)
  * Specify how much Hugepage memory is assigned to Lagopus in mega
  byte.
  Maximum hugepage memory is assigned if no option is specified.
  * Example: Use 512MB for Lagopus

		```
		-m 512
		```

* _--socket-mem_ : (Optional)
  * Specify how much Hugepage memory for each CPU socket is assigned
  to Lagopus in mega byte.
  This option is exclusive with _-m_ option

### Datapath option

#### Common option

* _--no-cache_ :
	* Don't use flow cache
* _--fifoness TYPE_ :
  * Select packet ordering (FIFOness) mode [default: none]
  * _none_ :
    * FIFOness is disabled
  * _port_ :
    * FIFOness per each port.
  * _flow_ :
    * FIFOness per each flow.
  * Example: Specify flow-level FIFOness

		```
		--fifoness flow
		```

* _--hashtype TYPE_ :
  * Select key-value store type for flow cache [default: intel64]
    * _intel64_	 Hash with Intel CRC32 and XOR (64bit)
	* _city64_	CityHash (64bit)
	* _murmur3_	 MurmurHash3 (32bit)
  * Example: Use CityHash

		```
		--hashtype city64
		```

* _--kvstype TYPE_:
  *  Select key-value store type for flow cache [default: hashmap_nolock]
    * hashmap_nolock:
      * Use hashmap without reader and writer lock
    * hashmap:
      * Use hashmap with reader and writer lock
    * ptree:
      * Use patricia tree
  * Example: Use patricia tree for key-value store

		```
		--kvstype ptree
		```

#### CPU core and packet processing
Dataplane of Lagopus provides two options to assign CPU core and
packet processing worker.
These options are exclusive.

##### Automatic assignment option
* _-p PORTMASK_ : (mandatory)
  * hexadecimal bitmask of ports to be used
  * Example: Use port 0 and port 1

		```
		-p3
		```

* _-w NWORKERS_ :
  * number of packet processing worker  [default: assign as possible]
  * System assigns CPU core automatically.
  * Example: Use total 8 cores for data plane

		```
		-w8
		```

##### Explicit assignment option
Current Lagopus limitation: youngest core number can not be specified
among available CPU cores.

* _--rx "(PORT, QUEUE, CORE),(PORT, QUEUE, CORE),...,(PORT, QUEUE, CORE)"_ :
  * List NIC RX assignment policy of NIC RX port, queue, and CPU core with the
  combination of (PORT, QUEUE, CORE) and ","
  * PORT:
	* Port number [0 - n]
  * QUEUE:
	* Queue number [default: 0]
  * CORE:
	* CPU core number

  * Example: Assign port 0 to CPU core #2 and port 1 to CPU core #3

		```
			--rx '(0,0,2),(1,0,3)'
		```

* _--tx	"(PORT,CORE),(PORT,CORE),...,(PORT,CORE)"_ :
  * List NIC TX assignment policy of NIC TX port and CPU core with the
  combination of (PORT, CORE) and ","
  * PORT:
    * Port number [0 - n]
  * CORE:
	* CPU core number

  * Example: Assign port 0 TX to CPU core #4 and port 1 TX to CPU core #5

		```
			--tx '(0,4),(1,5)'
		```

* _--w "CORE, ..., CORE"_ :
  * List of the CPU cores for packet processing with ","
  * Example: Assign CPU core 8 - 15 for packet processing

		```
			--w '8,9,10,11,12,13,14,15'
		```

CLI
--------------------------
Lagopus provides CLI for management.
Current implementation is limited.
Show port, flow table, controller and bridge name are supported


```
sudo lagopus/src/config/cli/lagosh
```

Development
==========================
Lagopus has the developer's mode build to proceed unit tests in Lagopus.
Details can be found docs/how-to-write-tests-in-lagopus.md.

You can enable developer mode with the following configure option.

	$ configure with --enable-developer

To conduct developer's mode, you should install the following packages:

* ruby
* Unity (a unit test framework)
* gcovr
