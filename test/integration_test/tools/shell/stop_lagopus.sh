#!/bin/sh

# ./stop_lagopus.sh <LOG_DIR> <TIME_OUT>

LOG_DIR=${1:-"."}
PID_FILE="/var/run/lagopus.pid"
RETRY_MAX=${2:-30}

if [ -f "${PID_FILE}" ]; then
    PID=`cat ${PID_FILE}`

    # kill
    sudo -E kill -TERM $PID

    i=0
    while :
    do
        IS_ALIVE=`ps hp ${PID} | wc -l`
        if [ "x${IS_ALIVE}" = "x0" ]; then
            exit 0
        fi
        if [ $i -ge $RETRY_MAX ]; then
            echo "Time out."
            exit 1;
        fi
        sleep 1
        i=`expr $i + 1`
        echo "check retry: ${i}." 1>&2
    done
fi
