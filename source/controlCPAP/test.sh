#!/bin/sh


((testTimes=0))
while true
do
    echo -e "\x93\x47\n" | ./a.out 18
    if [ ! $? -eq 0 ]
    then
        exit 1
    fi
    ((testTimes++))
    printf "test %d\n" $testTimes

done
