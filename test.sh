#!/bin/bash

sleep 1

for i in {1..100}; do
    ./client usr_$i 1 &
    pids[$i]=$!
done

echo "... attendo"

for pid in ${pids[*]}; do
    wait $pid
done

echo "test OK!"
exit 0
