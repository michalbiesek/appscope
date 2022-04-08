#!/bin/bash
# Requires nginx, scope, ab
# Configure /etc/nginx/nginx.conf with: worker_processes 1

NUMBER_OP=10000

print_summary() {
    printf "Start:\t%s\n" $START 
    printf "End:\t%s\n" $END 
    printf "Diff:\t%s\n" $(($END-$START)) 
    printf "Memory Info:\t%s\n" $MEM_INFO 
}

stop_nginx() {
    if [ -e /var/run/nginx.pid ]; then
        sudo nginx -s stop
    fi
}

unscoped_nginx() {
    printf "Unscoped Nginx:\n"
    stop_nginx
    sudo nginx
    PID=$(ps -ef | grep www-data | grep -v grep | awk '{ print $2 }')
    START=$(cat /proc/$PID/schedstat | awk '{ print $1 }')
    ab -c 1 -n $NUMBER_OP http://127.0.0.1:80/ > /dev/null 2>&1
    END=$(cat /proc/$PID/schedstat | awk '{ print $1 }')
    MEM_INFO=$(sudo pmap $PID | tail -n 1 | awk '/[0-9]K/{print $2}')
    print_summary
}

scoped_nginx() {
    printf "\nScoped Nginx:\n"
    stop_nginx
    sudo ../../../bin/linux/x86_64/scope run -- nginx
    PID=$(ps -ef | grep www-data | grep -v grep | awk '{ print $2 }')
    START=$(cat /proc/$PID/schedstat | awk '{ print $1 }')
    ab -c 1 -n $NUMBER_OP http://127.0.0.1:80/ > /dev/null 2>&1
    END=$(cat /proc/$PID/schedstat | awk '{ print $1 }')
    MEM_INFO=$(sudo pmap $PID | tail -n 1 | awk '/[0-9]K/{print $2}')
    print_summary
}

unscoped_nginx
scoped_nginx
stop_nginx