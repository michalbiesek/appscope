#! /bin/bash

DEBUG=0  # set this to 1 to capture the EVT_FILE for each test

FAILED_TEST_LIST=""
FAILED_TEST_COUNT=0

EVT_FILE="/opt/test-runner/logs/events.log"
TEMP_OUTPUT_FILE="/opt/test-runner/temp_output"
DUMMY_FILTER_FILE="/opt/test-runner/dummy_filter"
touch $EVT_FILE
touch $TEMP_OUTPUT_FILE

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
# Detach unscoped process (library is not loaded)
# 
starttest detachNotScopedProcessLibNotLoaded

top -b -d 1 > /dev/null &
TOP_PID=`pidof top`
ldscope --detach $TOP_PID 2> $TEMP_OUTPUT_FILE
if [ $? -eq 0 ]; then
    ERR+=1
fi

grep "error: pid $TOP_PID has never been attached" $TEMP_OUTPUT_FILE > /dev/null
if [ $? -ne 0 ]; then
    ERR+=1
fi
kill -9 $TOP_PID

endtest

#
# Detach unscoped process (library is loaded)
# 

starttest detachNotScopedProcessLibLoaded

SCOPE_FILTER=$DUMMY_FILTER_FILE ldscope top -b -d 1 > /dev/null &
sleep 1
TOP_PID=`pidof top`

THREAD_NO=`ls /proc/$TOP_PID/task | wc -l`
if [ $THREAD_NO -ne 1 ]; then
    echo "Error: Reporting thread is running in loaded & unscoped state"
    ERR+=1
fi

ldscope --detach $TOP_PID > $TEMP_OUTPUT_FILE
if [ $? -eq 0 ]; then
    ERR+=1
fi

# TODO this should be improved later when we will be able to see the state
# And do not create a file
grep "Detaching from pid $TOP_PID" $TEMP_OUTPUT_FILE > /dev/null
if [ $? -ne 0 ]; then
    ERR+=1
fi

ldscope --attach $TOP_PID > $TEMP_OUTPUT_FILE
grep "Reattaching to pid $TOP_PID" $TEMP_OUTPUT_FILE > /dev/null
if [ $? -ne 0 ]; then
    ERR+=1
fi

sleep 1
THREAD_NO=`ls /proc/$TOP_PID/task | wc -l`
if [ $THREAD_NO -ne 2 ]; then
    echo "Error: Reporting thread is not running after attach"
    ERR+=1
fi

kill -9 $TOP_PID

endtest

#
# Top
#
starttest Top

top -b -d 1 > /dev/null &
sleep 1
TOP_PID=`pidof top`
ldscope --attach $TOP_PID
sleep 1
evaltest

grep '"proc":"top"' $EVT_FILE | grep fs.open > /dev/null
ERR+=$?

grep '"proc":"top"' $EVT_FILE | grep fs.close > /dev/null
ERR+=$?

ldscope --detach $TOP_PID
if [ $? -eq 0 ]; then
    ERR+=1
fi
sleep 5
# Check file size

ldscope --attach $TOP_PID
sleep 5


# Check file size


kill -9 $TOP_PID

endtest

#
# Python3 Web Server
#
starttest Python3

python3 -m http.server 2> /dev/null &
sleep 1
ldscope --attach `pidof python3`
curl http://localhost:8000
sleep 1
evaltest

grep -q '"proc":"python3"' $EVT_FILE > /dev/null
ERR+=$?

grep -q http.req $EVT_FILE > /dev/null
ERR+=$?

grep -q http.resp $EVT_FILE > /dev/null
ERR+=$?

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
