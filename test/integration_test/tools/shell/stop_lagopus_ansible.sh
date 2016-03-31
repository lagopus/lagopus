#!/bin/bash

# ./stop_lagopus_ansible.sh <USER> <HOST> <LAGOPUS PATH> [<TIMEOUT>] [<LOG_DIR>] [LOCAL_LOG_DIR] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]

LAGOPUS=$1
USER=$2
HOST=$3
TIMEOUT=${4:-30}
LOG_DIR=${5:-"."}
LOCAL_LOG_DIR=${6:-""}
LAGOPUS_LOG=$LOG_DIR/${7:-"lagopus.log"}
LAGOPUS_OPTS=${@:8}
PID_FILE="/var/run/lagopus.pid"
WD=$(cd $(dirname $0); pwd)

usage() {
    echo "Usage: ${0} <USER> <HOST> <LAGOPUS PATH> [<TIMEOUT>] [IS_REMOTE] [<LOG_DIR>] [<LAGOPUS_LOG>] [<LAGOPUS_OPTS>]" 1>&2
}

VARS=""
VARS+=" lagopus=${LAGOPUS}"
VARS+=" lagopus_log=${LAGOPUS_LOG}"
VARS+=" lagopus_opts='${LAGOPUS_OPTS}'"
VARS+=" pid_file=${PID_FILE}"
VARS+=" timeout=${TIMEOUT}"
VARS+=" action=stop"

if [ x"${LOCAL_LOG_DIR}" != x"" ]; then
    VARS+=" lagopus_local_log=${LOCAL_LOG_DIR}"
    VARS+=" is_remote=true"
fi

echo $VARS

ansible-playbook $WD/../ansible/lagopus.yml --extra-vars "${VARS}" -u $USER -i "${HOST},"

RET=$?

exit $RET
