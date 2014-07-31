Sample configurations for Ryu Certification
===========================================
Ryu has a test tool for OpenFlow switches.
Detailed information can be found in [Ryu Book](https://github.com/osrg/ryu-book).
Lagopus can act as both a test target switch and an auxiliary switch.
This directory contains sample configuration files for the both.

* lagopus_dut.conf
 - For test target switch
* lagopus_aux.conf
 - For auxiliary switch

The difference between the above files is DPID.
According to the Ryu book, Ryu distinguishes the switches by DPID.
