#! /bin/bash
export SCOPE_EVENT_DEST=file:///opt/test/logs/events.log

DEBUG=0  # set this to 1 to capture the EVT_FILE for each test
FAILED_TEST_LIST=""
FAILED_TEST_COUNT=0

starttest(){
    CURRENT_TEST=$1
    echo "=============================================="
    echo "             Testing $CURRENT_TEST            "
    echo "=============================================="
    ERR=0
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

    echo "******************* $RESULT *******************"
    echo ""
    echo ""
}

run() {
    CMD="$@"
    echo "\`${CMD}\`"
    OUT=$(${CMD} 2>&1)
    RET=$?
}

outputs() {
    if ! grep "$1" <<< "$OUT" >/dev/null; then
        echo "FAIL: Expected \"$1\" in output of \`$CMD\`, got $OUT"
        ERR+=1
    else
	echo "PASS: Output as expected"
    fi
}

doesnt_output() {
    if grep "$1" <<< "$OUT" >/dev/null; then
        echo "FAIL: Didn't expect \"$1\" in output of \`$CMD\`"
        ERR+=1
    else
	echo "PASS: Output as expected"
    fi
}

################# START TESTS ################# 


#
# Scope ps
#
starttest "Scope ps"

python3 -m http.server 9090 &
python3_disabled=$!
SCOPE_FILTER=$DUMMY_FILTER_FILE ldscope -- python3 -m http.server 9091 &
python3_disabled=$!
python3 -m http.server 9092 &
python3_disabled=$!
python3 -m http.server 9093 &
python3_disabled=$!

# Scope ps
run scope ps
outputs "ID	PID	USER	COMMAND
1	${sleep_pid} 	root	sleep 1000"
returns 0

endtest


################# END TESTS ################# 

#
# Print test results
#
if (( $FAILED_TEST_COUNT == 0 )); then
    echo ""
    echo ""
    echo "************ ALL CLI TESTS PASSED ************"
else
    echo "************ SOME CLI TESTS FAILED ************"
    echo "Failed tests: $FAILED_TEST_LIST"
fi
echo ""

exit ${FAILED_TEST_COUNT}
