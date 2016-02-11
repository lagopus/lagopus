#!/bin/bash

# ./dummy_eth_down.sh [<ETH_NAME>] [<REPETITION_COUNT>]

ETH_NAME=${1:-"dummy_eth"}
REPETITION_COUNT=${2:-1}
PWD=$(cd $(dirname $0); pwd)

usage() {
    echo "Usage: ${0}  [<ETH_NAME>] [<REPETITION_COUNT>]" 1>&2
}

N=`expr $REPETITION_COUNT - 1`
for i in `seq 0 $N`
do
    IS_EXISTS=`ip link show ${ETH_NAME}${i} 2>/dev/null | wc -l`
    if [ $IS_EXISTS -eq 0 ]; then
        continue
    fi

    echo "dummy eth: ${ETH_NAME}${i} down... "
    sudo ip link set "${ETH_NAME}${i}" down && \
        sudo ip link delete "${ETH_NAME}${i}"

    RET=$?
    if [ $RET -ne 0 ]; then
        exit $RET
    fi
done
