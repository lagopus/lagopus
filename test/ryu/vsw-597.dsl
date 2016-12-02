interface interface0 create -type ethernet-rawsock -device veth0
port port01 create -interface interface0
bridge bridge01 create -port port01 1 -dpid 0x1 -fail-mode standalone
bridge bridge01 enable
