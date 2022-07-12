#!/bin/bash

ERR=0

fail() { ERR+=1; echo >&2 "fail:" $@; }

rc-service nginx start

#Check modified configuration
scope service nginx --force --cribldest tls://example_instance.cribl.cloud:10090

if [ ! -f /etc/scope/nginx/scope.yml ]; then
    fail "missing scope.yml"
fi

if [ ! -f /etc/conf.d/nginx ]; then
    fail "missing /etc/conf.d/nginx"
fi

if [ ! -d /var/log/scope ]; then
    fail "missing /var/log/scope/"
fi

if [ ! -d /var/run/scope ]; then
    fail "missing /var/run/scope/"
fi

count=$(grep 'export LD_PRELOAD' /etc/conf.d/nginx | wc -l)
if [ $count -ne 1 ] ; then
    fail "missing LD_PRELOAD in /etc/conf.d/nginx"
fi

count=$(grep 'export SCOPE_HOME' /etc/conf.d/nginx | wc -l)
if [ $count -ne 1 ] ; then
    fail "missing SCOPE_HOME in /etc/conf.d/nginx"
fi

count=$(grep 'example_instance' /etc/scope/nginx/scope.yml | wc -l)
if [ $count -ne 1 ] ; then
    fail "Wrong configuration in scope.yml"
fi

# #Check default configuration
scope service nginx --force

count=$(grep 'example_instance' /etc/scope/nginx/scope.yml | wc -l)
if [ $count -ne 0 ] ; then
    fail "Wrong configuration in scope.yml"
fi

if [ $ERR -gt 0 ]; then
    echo "$ERR test(s) failed"
    exit $ERR
else
    echo "All test passed"
fi
