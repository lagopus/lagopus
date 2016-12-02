channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main
channel channel02 create -dst-addr 127.0.0.1 -protocol tcp
controller controller02 create -channel channel02 -role equal -connection-type main

interface interface0 create -type ethernet-dpdk-phy -device "eth_pipe0"
interface interface1 create -type ethernet-dpdk-phy -device "eth_pipe1,attach=eth_pipe0"
port port01 create -interface interface0
port port02 create -interface interface1

bridge bridge01 create -controller controller01 -port port01 1 -dpid 0x1
bridge bridge01 enable
bridge bridge02 create -controller controller02 -port port02 1 -dpid 0x2
bridge bridge02 enable
