#!/bin/sh

# ./start_lagopus.sh <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]

LAGOPUS=$1
LOG_DIR=${2:-"."}
LAGOPUS_LOG=$LOG_DIR/${3:-"lagopus.log"}
LAGOPUS_OPTS=$4
PID_FILE="/var/run/lagopus.pid"
LAGOPUS_LOG_DEBUGLEVEL=${LAGOPUS_LOG_DEBUGLEVEL:-1}
PROC_STATUS_FILE="${LOG_DIR}/begin_proc_status.txt"
PWD=$(cd $(dirname $0); pwd)

usage() {
    echo "Usage: ${0} <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]" 1>&2
}

$PWD/stop_lagopus.sh $LOG_DIR

if [ "x${LAGOPUS}" = "x" ]; then
    echo "ERROR : Invalid args." 1>&2
    usage
    exit 1;
fi

# run
export LAGOPUS_LOG_DEBUGLEVEL
sudo -E $LAGOPUS -l $LAGOPUS_LOG $LAGOPUS_OPTS -p $PID_FILE
RET=$?

# save proc
PID=`cat ${PID_FILE}`
sudo cat /proc/$PID/status > $PROC_STATUS_FILE

exit $RET
