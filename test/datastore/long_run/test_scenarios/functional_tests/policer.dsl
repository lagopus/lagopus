policer-action policer-action01 create -type discard
policer-action policer-action02 create -type discard

policer policer01 create -action policer-action01 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0
policer policer02 create -action policer-action02 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0

interface if01 create -type ethernet-rawsock -device lago_eth0 -mtu 1 -ip-addr 127.0.0.1

port port01 create -interface if01 -policer policer01

channel channel01 create -dst-addr 127.0.0.1 -protocol tcp

controller controller01 create -channel channel01 -role equal -connection-type main

bridge bridge01 create -controller controller01 -port port01 1 -dpid 0x1
