interface if01 create -type ethernet-rawsock -device lago_eth0
interface if02 create -type ethernet-rawsock -device lago_eth1
interface if03 create -type ethernet-rawsock -device lago_eth2
interface if04 create -type ethernet-rawsock -device lago_eth3

port port01 create -interface if01
port port02 create -interface if02
port port03 create -interface if03
port port04 create -interface if04

channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
channel channel02 create -dst-addr 127.0.0.1 -protocol tcp

controller controller01 create -channel channel01 -role equal -connection-type main
controller controller02 create -channel channel02 -role equal -connection-type main

bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -dpid 0x1
bridge bridge02 create -controller controller02 -port port03 1 -port port04 2 -dpid 0x2
