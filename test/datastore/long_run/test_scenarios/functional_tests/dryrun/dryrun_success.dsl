channel channel01 create -dst-addr 127.0.0.1 -dst-port 12345 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main
interface if01 create -type ethernet-rawsock -device lago_eth0 -mtu 1 -ip-addr 127.0.0.1
interface if02 create -type ethernet-rawsock -device lago_eth1 -mtu 2 -ip-addr 127.0.0.2
queue queue01 create -type single-rate -priority 2 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
queue queue02 create -type single-rate -priority 3 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
policer-action policer-action01 create -type discard
policer-action policer-action02 create -type discard
policer policer01 create -action policer-action01 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0
policer policer02 create -action policer-action02 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0
port port01 create -interface if01 -policer policer01 -queue queue01 1
port port02 create -interface if02 -policer policer02 -queue queue02 2
bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -dpid 0x1
