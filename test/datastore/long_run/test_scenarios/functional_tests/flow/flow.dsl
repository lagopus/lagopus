interface if01 create -type ethernet-rawsock -port-number 1 -device lago_eth0
interface if02 create -type ethernet-rawsock -port-number 2 -device lago_eth2

port port01 create -interface if01
port port02 create -interface if02

channel channel01 create -dst-addr 127.0.0.1 -protocol tcp

controller controller01 create -channel channel01 -role equal -connection-type main

bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -dpid 0x1
