channel channel01 create -dst-addr 192.168.1.58 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main
channel channel02 create -dst-addr 192.168.1.58 -protocol tcp
controller controller02 create -channel channel02 -role equal -connection-type main

interface interface0 create -type ethernet-dpdk-phy -device "net_pipe0"
interface interface1 create -type ethernet-dpdk-phy -device "net_pipe1,attach=net_pipe0"
interface interface2 create -type ethernet-dpdk-phy -device "net_pipe2"
interface interface3 create -type ethernet-dpdk-phy -device "net_pipe3,attach=net_pipe2"
interface interface4 create -type ethernet-dpdk-phy -device "net_pipe4"
interface interface5 create -type ethernet-dpdk-phy -device "net_pipe5,attach=net_pipe4"

port port01 create -interface interface0
port port02 create -interface interface1
port port03 create -interface interface2
port port04 create -interface interface3
port port05 create -interface interface4
port port06 create -interface interface5

bridge bridge01 create -controller controller01 -port port01 1 -port port03 2 -port port05 3 -dpid 0x1
bridge bridge01 enable
bridge bridge02 create -controller controller02 -port port02 1 -port port04 2 -port port06 3 -dpid 0x2
bridge bridge02 enable
