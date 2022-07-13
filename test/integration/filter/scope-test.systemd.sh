#!/bin/bash

ERR=0

fail() { ERR+=1; echo >&2 "fail:" $@; }

scope filter

if [ $ERR -gt 0 ]; then
    echo "$ERR test(s) failed"
    exit $ERR
else
    echo "All test passed"
fi
