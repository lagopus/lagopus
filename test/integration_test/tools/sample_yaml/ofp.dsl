# DSL
channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
controller controller01 create -channel channel01
interface interface01 create -type ethernet-dpdk-phy -port-number 0
interface interface02 create -type ethernet-dpdk-phy -port-number 1
interface interface03 create -type ethernet-dpdk-phy -port-number 2
port port01 create -interface interface01
port port02 create -interface interface02
port port03 create -interface interface03
bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -port port03 3 -dpid 0x1
bridge bridge01 enable
