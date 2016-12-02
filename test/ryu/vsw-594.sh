#!/bin/sh
ryu-manager ./vsw-594.py &
RYU_PID=$!
trap "kill $RYU_PID"
sudo lagopus -d -C ./vsw-594.dsl -- -cff -n2 --
