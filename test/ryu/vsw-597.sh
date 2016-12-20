#!/bin/sh -v
sudo ip link add veth0 type veth peer name veth1
sudo lagopus -l /dev/stdout -d -C ./vsw-597.dsl &
sleep 1
sudo ip link delete veth0
echo "bridge bridge01 config -port ~port01" | nc localhost 12345
lagosh -c show interface
lagosh -c stop
