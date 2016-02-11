#!/bin/sh

echo "START: insmod"

sudo /sbin/modprobe uio
sudo /sbin/insmod $RTE_SDK/build/kmod/igb_uio.ko
sudo /sbin/insmod $RTE_SDK/build/kmod/rte_kni.ko
