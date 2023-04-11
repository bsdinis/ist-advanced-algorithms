#!/usr/bin/env zsh

RED="\033[0;31m"
GREEN="\033[0;32m"
RESET="\033[0m"

for i in {0..100}
do
    echo -n "Test ${i} "
    ./fuzz.py > tests/TFUZZ/input
    ./sol.py < tests/TFUZZ/input > tests/TFUZZ/output
    ./project < tests/TFUZZ/input > tests/TFUZZ/actual_output

    diff tests/TFUZZ/output tests/TFUZZ/actual_output

    if [ $? -eq 0 ]
    then
        echo "${GREEN}OK${RESET}";
    else
        echo "${RED}NOK${RESET}";
    fi
done