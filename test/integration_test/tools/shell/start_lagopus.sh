#!/bin/bash

# ./start_lagopus.sh <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]

LAGOPUS=$1
LOG_DIR=${2:-"."}
LAGOPUS_LOG=$LOG_DIR/${3:-"lagopus.log"}
LAGOPUS_OPTS=${@:4}
PID_FILE="/var/run/lagopus.pid"
LAGOPUS_LOG_DEBUGLEVEL=${LAGOPUS_LOG_DEBUGLEVEL:-1}
PWD=$(cd $(dirname $0); pwd)
GO_MSG="lagopus is a go\."

usage() {
    echo "Usage: ${0} <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]" 1>&2
}

sudo -E rm -f $LAGOPUS_LOG

$PWD/stop_lagopus.sh $LOG_DIR

if [ "x${LAGOPUS}" = "x" ]; then
    echo "ERROR : Invalid args." 1>&2
    usage
    exit 1;
fi

# run
export LAGOPUS_LOG_DEBUGLEVEL
sudo -E $LAGOPUS -l $LAGOPUS_LOG -p $PID_FILE $LAGOPUS_OPTS
RET=$?

i=0
while :
do
    IS_GO=`sh -c "sudo -E grep '${GO_MSG}' ${LAGOPUS_LOG} | wc -l"`
    if [ "x${IS_GO}" != "x0" ]; then
        exit 0
    fi
    sleep 1
    i=`expr $i + 1`
    echo "check retry: ${i}." 1>&2
done

exit $RET
