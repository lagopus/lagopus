# DSL
channel channel01 create -dst-addr {{ switches.target.host }} -protocol tcp
controller controller01 create -channel channel01
interface interface01 create -type ethernet-dpdk-phy -port-number 0
interface interface02 create -type ethernet-dpdk-phy -port-number 1
interface interface03 create -type ethernet-dpdk-phy -port-number 2
queue queue01 create -type single-rate -id 1 -priority 2 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
queue queue02 create -type single-rate -id 2 -priority 3 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
queue queue03 create -type single-rate -id 3 -priority 4 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
port port01 create -interface interface01 -queue queue01
port port02 create -interface interface02 -queue queue02
port port03 create -interface interface03 -queue queue03
bridge bridge01 create -controller controller01 -port port01 1 -port port02 2 -port port03 3 -dpid {{ switches.target.dpid }}
bridge bridge01 enable
