queue queue01 create -type single-rate -priority 2 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
queue queue02 create -type single-rate -priority 3 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500

interface if01 create -type ethernet-rawsock -device lago_eth0 -mtu 1 -ip-addr 127.0.0.1

port port01 create -interface if01 -queue queue01 1

channel channel01 create -dst-addr 127.0.0.1 -protocol tcp

controller controller01 create -channel channel01 -role equal -connection-type main

bridge bridge01 create -controller controller01 -port port01 1 -dpid 0x1
