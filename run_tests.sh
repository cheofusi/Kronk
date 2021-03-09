#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

NUM_FAILED_TESTS=0
for test in $(ls tests/); do
    out=$(bin/kronkc $test)
    failed=$(echo $out | tr ' ' '\n' | grep -c "faux")

    if (($failed)); then
        printf "%s ${RED}%s${NC}\n" $test "FAILED"
        ((NUM_FAILED_TESTS++))
    else
        printf "%s ${GREEN}%s${NC}\n" $test "PASSED"
    fi
done

[ $NUM_FAILED_TESTS -ne 0 ] && exit 1
exit 0
