#!/bin/sh

echo "START: hugepages"

sudo mkdir -p /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge || _error
sudo sh -c "echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages" || _error
