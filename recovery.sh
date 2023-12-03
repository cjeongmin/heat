#!/bin/bash

echo "RECOVERY START"

echo "HEAT_FAIL_CODE: $HEAT_FAIL_CODE"
echo "HEAT_FAIL_TIME: $HEAT_FAIL_TIME"
echo "HEAT_FAIL_LAST: $HEAT_FAIL_LAST"
echo "HEAT_FAIL_INTERVAL: $HEAT_FAIL_INTERVAL"
echo "HEAT_FAIL_PID: $HEAT_FAIL_PID"
echo "HEAT_FAIL_CNT: $HEAT_FAIL_CNT"

sleep 3

echo "RECOVERY END"


# ./heat -i 10 -s script.sh --fail failure.sh --recovery recovery.sh --threshold 2 --recovery-timeout 15