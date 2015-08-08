#!/bin/sh

# ./dummy_eth_up.sh [<ETH_NAME>] [<REPETITION_COUNT>]

ETH_NAME=${1:-"dummy_eth"}
REPETITION_COUNT=${2:-1}
PWD=$(cd $(dirname $0); pwd)

usage() {
    echo "Usage: ${0}  [<ETH_NAME>] [<REPETITION_COUNT>]" 1>&2
}

$PWD/dummy_eth_down.sh $ETH_NAME $REPETITION_COUNT

N=`expr $REPETITION_COUNT - 1`
for i in `seq 0 $N`
do
    echo "dummy eth: ${ETH_NAME}${i} up... "
    sudo ip link add "${ETH_NAME}${i}" type dummy && \
        sudo ip link set "${ETH_NAME}${i}" up

    RET=$?
    if [ $RET -ne 0 ]; then
        exit $RET
    fi
done
