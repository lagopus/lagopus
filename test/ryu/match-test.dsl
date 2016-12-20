channel channel01 create -dst-addr 127.0.0.1 -protocol tcp
controller controller01 create -channel channel01 -role equal -connection-type main

bridge bridge01 create -controller controller01 -dpid 0x1
bridge bridge01 enable
