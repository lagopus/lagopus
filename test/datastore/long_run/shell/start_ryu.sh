#!/bin/sh

# ./start_ryu.sh <RYU PATH> [<LOG_DIR>] [<RYU_LOG>] [<RYU_OPTS>]
RYU=$1
LOG_DIR=${2:-"."}
RYU_LOG=$LOG_DIR/${3:-"./ryu.log"}
RYU_OPTS=$4
PID_FILE="/var/run/ryu.pid"
PWD=$(cd $(dirname $0); pwd)

usage() {
    echo "Usage: ${0} <RYU PATH> [<LOG_DIR>] [<RYU_LOG>] [<RYU_OPTS>]" 1>&2
}

$PWD/stop_ryu.sh $LOG_DIR

if [ "x${RYU}" = "x" ]; then
    echo "ERROR : Invalid args." 1>&2
    usage
    exit 1;
fi

$RYU --log-file $RYU_LOG $RYU_OPTS --pid-file $PID_FILE > /dev/null 2>&1 &
