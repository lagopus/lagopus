#!/bin/sh

# usage:
#  ./bind.sh [PCI_NO [PCI_NO] ...]
#

echo "START: bind"

if [ $# -eq 0 ]; then
    OPTS=`sudo lshw -class network -businfo -numeric | grep "8086:100E" | \
          sed -e '1,2'd|awk '{sub(/pci@/, "", $1); printf("%s ", $1)}'`
else
    OPTS=$@
fi

for OPT in $OPTS; do
    echo "$OPT"

    sudo $RTE_SDK/tools/dpdk_nic_bind.py -b e1000 $OPT
done
