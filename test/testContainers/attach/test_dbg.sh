#! /bin/bash

cd /opt/busybox-1.33.1/
./busybox_unstripped top -b -d 1 > /dev/null &
ldscope --attach `pidof busybox_unstripped`
