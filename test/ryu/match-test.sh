#!/bin/sh
ryu-manager $1.py &
RYU_PID=$!
trap "kill $RYU_PID"
sudo lagopus -d -C ./match-test.dsl -- -cff -n2 --
