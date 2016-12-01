# all the log objects' attribute
log -packetdump ""

# all the datastore objects' attribute
datastore -addr 0.0.0.0 -port 12345 -protocol tcp -tls false

# all the agent objects' attribute
agent -channelq-size 1000 -channelq-max-batches 1000

# all the tls objects' attribute
tls -cert-file /usr/local/etc/lagopus/catls.pem -private-key /usr/local/etc/lagopus/key.pem -certificate-store /usr/local/etc/lagopus -trust-point-conf /usr/local/etc/lagopus/check.conf

# all the snmp objects' attribute
snmp -master-agentx-socket tcp:localhost:705 -ping-interval-second 10

# all the namespace objects' attribute

# all the policer-action objects' attribute
policer-action :policer-action01 create -type discard
policer-action :policer-action02 create -type discard

# all the policer objects' attribute
policer :policer01 create -action :policer-action01 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0
policer :policer02 create -action :policer-action02 -bandwidth-limit 1500 -burst-size-limit 1500 -bandwidth-percent 0

# all the queue objects' attribute
queue :queue01 create -type single-rate -priority 2 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500
queue :queue02 create -type single-rate -priority 3 -color color-aware -committed-burst-size 1500 -committed-information-rate 1500 -excess-burst-size 1500

# all the interface objects' attribute
interface :if01 create -type ethernet-rawsock -device lago_eth0 -mtu 1 -ip-addr 127.0.0.1
interface :if02 create -type ethernet-rawsock -device lago_eth1 -mtu 2 -ip-addr 127.0.0.2

# all the port objects' attribute
port :port01 create -interface :if01 -policer :policer01 -queue :queue01 1
port :port02 create -interface :if02 -policer :policer02 -queue :queue02 2

# all the channel objects' attribute
channel :channel01 create -dst-addr 127.0.0.1 -dst-port 12345 -local-addr 0.0.0.0 -local-port 0 -protocol tcp

# all the controller objects' attribute
controller :controller01 create -channel :channel01 -role equal -connection-type main

# all the bridge objects' attribute
bridge :bridge01 create -dpid 1 -controller :controller01 -port :port01 1 -port :port02 2 -fail-mode secure -flow-statistics true -group-statistics true -port-statistics true -queue-statistics true -table-statistics true -reassemble-ip-fragments false -max-buffered-packets 65535 -max-ports 255 -max-tables 255 -max-flows 4294967295 -packet-inq-size 1000 -packet-inq-max-batches 1000 -up-streamq-size 1000 -up-streamq-max-batches 1000 -down-streamq-size 1000 -down-streamq-max-batches 1000 -block-looping-ports false

# policer-action objects' status
policer-action :policer-action01 disable
policer-action :policer-action02 disable

# policer objects' status
policer :policer01 disable
policer :policer02 disable

# queue objects' status
queue :queue01 disable
queue :queue02 disable

# interface objects' status
interface :if01 disable
interface :if02 disable

# port objects' status
port :port01 disable
port :port02 disable

# channel objects' status
channel :channel01 disable

# controller objects' status
controller :controller01 disable

# bridge objects' status
bridge :bridge01 disable

