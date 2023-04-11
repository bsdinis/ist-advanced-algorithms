#!/usr/bin/env zsh

total=0
success=0

RED="\033[0;31m"
GREEN="\033[0;32m"
RESET="\033[0m"

if [[ $# -eq 0 ]]
then
    for dir in tests/*;
    do
        total=$(echo $total + 1 | bc)
        echo -n "$(basename $dir)... "
        ./project < ${dir}/input > ${dir}/actual_output
        diff ${dir}/output ${dir}/actual_output > /dev/null 2> /dev/null;
        if [ $? -eq 0 ]
        then
            success=$(echo $success + 1 | bc)
            echo "${GREEN}OK${RESET}";
        else
            echo "${RED}NOK${RESET}";
        fi
    done
else
    for t in $@;
    do
        dir=$(printf "tests/T%02d" $t)
        echo -n "$(basename $dir)... "
        ./project < ${dir}/input > ${dir}/actual_output
        diff ${dir}/output ${dir}/actual_output;
        if [ $? -eq 0 ]
        then
            echo "${GREEN}OK${RESET}";
        else
            echo "${RED}NOK${RESET}";
        fi
    done
fi
