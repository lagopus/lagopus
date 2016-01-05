#!/bin/sh

# usage:
#  ./unbind.sh [PCI_NO,DEV_NAME [PCI_NO,DEV_NAME] ...]
#

echo "START: unbind"

if [ $# -eq 0 ]; then
    OPTS=`sudo lshw -class network -businfo -numeric | grep "8086:100E" | \
          sed -e '1,2'd|awk '{sub(/pci@/, "", $1); printf("%s,%s ", $1,$2)}'`
else
    OPTS=$@
fi

for OPT in $OPTS; do
    PCI=`echo $OPT | cut -d',' -f1`
    DEV=`echo $OPT | cut -d',' -f2`
    echo "$PCI,$DEV"

    sudo ifconfig $DEV down
    sudo $RTE_SDK/tools/dpdk_nic_bind.py --bind=igb_uio $PCI
done
