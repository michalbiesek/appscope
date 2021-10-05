#! /bin/bash

busybox top -b -d 1 > /dev/null &
PID=$!
ldscope --attach $PID
