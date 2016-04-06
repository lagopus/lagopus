#!/bin/sh

# ./stop_lagopus_remote.sh <USER>@<HOST> <LOCAL_LOG_DIR> [<LOG_DIR>] [<LAGOPUS_LOG>] [<TIME_OUT>]

SSH_USER_HOST=$1
LOCAL_LOG_DIR=$2
LOG_DIR=${3:-"."}
LAGOPUS_LOG_FILE=${4:-"lagopus.log"}
LAGOPUS_LOG=$LOG_DIR/$LAGOPUS_LOG_FILE
LAGOPUS_LOCAL_LOG=$LOCAL_LOG_DIR/$LAGOPUS_LOG_FILE
RETRY_MAX=${5:-30}
PID_FILE="/var/run/lagopus.pid"
SSH="ssh -T -oStrictHostKeyChecking=no"
SCP="scp -oStrictHostKeyChecking=no"

teardown() {
    # cp lagopus log : remote => local
    sudo -E $SSH $SSH_USER_HOST sudo -E cat ${LAGOPUS_LOG} > $LAGOPUS_LOCAL_LOG
    # remove lagopus log (remote)
    $SSH $SSH_USER_HOST sudo -E rm -f $LAGOPUS_LOG
}

if [ `${SSH} test -e ${PID_FILE}; echo \$?` != "0" ]; then
    PID=`${SSH} -n ${SSH_USER_HOST} cat ${PID_FILE}`

    # kill
    $SSH $SSH_USER_HOST sudo -E kill -TERM $PID

    i=0
    while :
    do
        IS_ALIVE=`${SSH} ${SSH_USER_HOST} ps hp ${PID} | wc -l`
        if [ "x${IS_ALIVE}" = "x0" ]; then
            teardown
            exit 0
        fi
        if [ $i -ge $RETRY_MAX ]; then
            echo "Time out."
            teardown
            exit 1;
        fi
        sleep 1
        i=`expr $i + 1`
        echo "check retry: ${i}." 1>&2
    done
fi
