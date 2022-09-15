#! /bin/bash

DEBUG=0  # set this to 1 to capture the EVT_FILE for each test

FAILED_TEST_LIST=""
FAILED_TEST_COUNT=0

EVT_FILE="/opt/test-runner/logs/events.log"
touch $EVT_FILE

starttest(){
    CURRENT_TEST=$1
    echo "==============================================="
    echo "             Testing $CURRENT_TEST             "
    echo "==============================================="
    ERR=0
}

evaltest(){
    echo "             Evaluating $CURRENT_TEST"
}

endtest(){
    if [ $ERR -eq "0" ]; then
        RESULT=PASSED
    else
        RESULT=FAILED
        FAILED_TEST_LIST+=$CURRENT_TEST
        FAILED_TEST_LIST+=" "
        FAILED_TEST_COUNT=$(($FAILED_TEST_COUNT + 1))
    fi

    echo "*************** $CURRENT_TEST $RESULT ***************"
    echo ""
    echo ""

    # copy the EVT_FILE to help with debugging
    if (( $DEBUG )) || [ $RESULT == "FAILED" ]; then
        cp -f $EVT_FILE $EVT_FILE.$CURRENT_TEST
    fi

    rm -f $EVT_FILE
}

wait_for_proc_start(){
    local proc_name=$1
    for i in `seq 1 8`;
    do
        if pidof $proc_name > /dev/null; then
            echo "Process $proc_name started"
            return
        fi
        echo "Sleep for the $i time because $proc_name did not started"
        sleep 1
    done

    echo "Process $proc_name did not started"
    ERR+=1
}

#
# Top
#
starttest Top

top -b -d 1 > /dev/null &
sleep 1
ldscope --attach `pidof top`
sleep 1
evaltest

grep '"proc":"top"' $EVT_FILE | grep fs.open > /dev/null
ERR+=$?

grep '"proc":"top"' $EVT_FILE | grep fs.close > /dev/null
ERR+=$?

kill -9 `pidof top`

endtest

#
# Python3 Web Server
#
starttest Python3

python3 -m http.server 2> /dev/null &
sleep 1
ldscope --attach `pidof python3`
ERR+=$?
curl http://localhost:8000
sleep 1
evaltest

grep -q '"proc":"python3"' $EVT_FILE > /dev/null
ERR+=$?

grep -q http.req $EVT_FILE > /dev/null
ERR+=$?

grep -q http.resp $EVT_FILE > /dev/null
ERR+=$?

# Detach
ldscope --detach `pidof python3`
ERR+=$?
# Ensure the detach is completed
sleep 5
EVE_FILE_SIZE_DETACH=”$(stat -c %s $EVT_FILE)
sleep 5
curl http://localhost:8000
sleep 1
if(($EVE_FILE_SIZE_DETACH !=`stat -c %s $EVT_FILE"`));then
  echo "Process was not detached"
  ERR+=1
fi

# Reattach
ldscope --attach `pidof python3`
ERR+=$?
sleep 5

kill -9 `pidof python3` > /dev/null

endtest

#
# Java HTTP Server
#
starttest java_http
cd /opt/java_http
java SimpleHttpServer 2> /dev/null &
sleep 1
ldscope --attach `pidof java`
curl http://localhost:8000/status
sleep 1
evaltest

grep -q '"proc":"java"' $EVT_FILE > /dev/null
ERR+=$?

grep -q http.req $EVT_FILE > /dev/null
ERR+=$?

grep -q http.resp $EVT_FILE > /dev/null
ERR+=$?

grep -q fs.open $EVT_FILE > /dev/null
ERR+=$?

grep -q fs.close $EVT_FILE > /dev/null
ERR+=$?

grep -q net.open $EVT_FILE > /dev/null
ERR+=$?

grep -q net.close $EVT_FILE > /dev/null
ERR+=$?

kill -9 `pidof java`

sleep 1

endtest

#
# attach execve_test
#

starttest execve_test

cd /opt/exec_test/
./exec_test 0 &

wait_for_proc_start "exec_test"
EXEC_TEST_PID=`pidof exec_test`

ldscope --attach ${EXEC_TEST_PID}
if [ $? -ne 0 ]; then
    echo "attach failed"
    ERR+=1
fi

wait ${EXEC_TEST_PID}
sleep 2

egrep '"cmd":"/usr/bin/curl -I https://cribl.io"' $EVT_FILE > /dev/null
if [ $? -ne 0 ]; then
    echo "Curl event not found"
    cat $EVT_FILE
    ERR+=1
fi

endtest

#
# attach execv_test
#

starttest execv_test

cd /opt/exec_test/
./exec_test 1 &

wait_for_proc_start "exec_test"
EXEC_TEST_PID=`pidof exec_test`

ldscope --attach ${EXEC_TEST_PID}
if [ $? -ne 0 ]; then
    echo "attach failed"
    ERR+=1
fi

wait ${EXEC_TEST_PID}
sleep 2

egrep '"cmd":"/usr/bin/wget -S --spider --no-check-certificate https://cribl.io"' $EVT_FILE > /dev/null
if [ $? -ne 0 ]; then
    echo "Wget event not found"
    cat $EVT_FILE
    ERR+=1
fi

endtest


if (( $FAILED_TEST_COUNT == 0 )); then
    echo ""
    echo ""
    echo "*************** ALL TESTS PASSED ***************"
else
    echo "*************** SOME TESTS FAILED ***************"
    echo "Failed tests: $FAILED_TEST_LIST"
    echo "Refer to these files for more info:"
    for FAILED_TEST in $FAILED_TEST_LIST; do
        echo "  $EVT_FILE.$FAILED_TEST"
    done
fi

exit ${FAILED_TEST_COUNT}
