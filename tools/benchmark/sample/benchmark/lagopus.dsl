interface interface01 create -type ethernet-rawsock -device eth0
interface interface02 create -type ethernet-rawsock -device eth1

port port01 create -interface interface01
port port02 create -interface interface02

bridge bridge01 create -port port01 1 -port port02 2 -dpid 0x1

flow bridge01 add priority=10 dl_src=00:00:00:00:00:01 apply_actions=output:2
