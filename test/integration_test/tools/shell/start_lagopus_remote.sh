#!/bin/bash

# ./start_lagopus_remote.sh <USER>@<HOST> <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]

SSH_USER_HOST=$1
LAGOPUS=$2
LOG_DIR=${3:-"."}
LAGOPUS_LOG=$LOG_DIR/${4:-"lagopus.log"}
LAGOPUS_OPTS=${@:5}
PID_FILE="/var/run/lagopus.pid"
LAGOPUS_LOG_DEBUGLEVEL=${LAGOPUS_LOG_DEBUGLEVEL:-1}
PWD=$(cd $(dirname $0); pwd)
GO_MSG="lagopus is a go\."
SSH="ssh -T -oStrictHostKeyChecking=no"

usage() {
    echo "Usage: ${0} <LAGOPUS PATH> [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]" 1>&2
}

$SSH $SSH_USER_HOST sudo -E rm -f $LAGOPUS_LOG

$PWD/stop_lagopus_remote.sh $SSH_USER_HOST $LOG_DIR

if [ "x${LAGOPUS}" = "x" ]; then
    echo "ERROR : Invalid args." 1>&2
    usage
    exit 1;
fi

# run
export LAGOPUS_LOG_DEBUGLEVEL
$SSH $SSH_USER_HOST sudo -E LAGOPUS_LOG_DEBUGLEVEL=$LAGOPUS_LOG_DEBUGLEVEL \
    $LAGOPUS -l $LAGOPUS_LOG -p $PID_FILE $LAGOPUS_OPTS
RET=$?

i=0
while :
do
    IS_GO=`${SSH} ${SSH_USER_HOST} sudo -E grep \"${GO_MSG}\" ${LAGOPUS_LOG} | wc -l`
    if [ "x${IS_GO}" != "x0" ]; then
        exit 0
    fi
    sleep 1
    i=`expr $i + 1`
    echo "check retry: ${i}." 1>&2
done

exit $RET
