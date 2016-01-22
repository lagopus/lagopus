channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main
channel channel02 create -dst-addr 127.0.0.1 -protocol tcp
controller controller02 create -channel channel02 -role equal -connection-type main

interface interface0 create -type ethernet-rawsock -device veth0
interface interface1 create -type ethernet-rawsock -device veth1
interface interface2 create -type ethernet-rawsock -device veth2
interface interface3 create -type ethernet-rawsock -device veth3
interface interface4 create -type ethernet-rawsock -device veth4
interface interface5 create -type ethernet-rawsock -device veth5

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
