#!/bin/bash
clients=0
totStores=0
totRetrives=0
totDeletes=0
totSuccess=0
totFailures=0
totStoredKB=0
totRetrivedKB=0
input="testout.log"
while IFS= read -r line; do
    f1=$(echo "$line" | cut -d ' ' -f 1)
    #client
    if [ "$(echo "${f1}" | grep "Client")" != "" ]; then
        ((clients += 1))
    fi
    #store
    if [ "$(echo "${f1}" | grep "Stores")" != "" ]; then
        stores=$(echo "$line" | cut -d ' ' -f 2)
        ((totStores += stores))
    fi
    #retrive
    if [ "$(echo "${f1}" | grep "Retrives")" != "" ]; then
        retrives=$(echo "$line" | cut -d ' ' -f 2)
        ((totRetrives += retrives))
    fi
    #delete
    if [ "$(echo "${f1}" | grep "Deletes")" != "" ]; then
        deletes=$(echo "$line" | cut -d ' ' -f 2)
        ((totDeletes += deletes))
    fi
    #success
    if [ "$(echo "${f1}" | grep "Successes")" != "" ]; then
        successes=$(echo "$line" | cut -d ' ' -f 2)
        ((totSuccess += successes))
    fi
    #failure
    if [ "$(echo "${f1}" | grep "Failures")" != "" ]; then
        failures=$(echo "$line" | cut -d ' ' -f 2)
        ((totFailures += failures))
    fi
    #stored KB
    if [ "$(echo "${f1}" | grep "Stored")" != "" ]; then
        storedKB=$(echo "$line" | cut -d ' ' -f 3)
        ((totStoredKB += storedKB))
    fi
    #retrived KB
    if [ "$(echo "${f1}" | grep "Retrived")" != "" ]; then
        retrivedKB=$(echo "$line" | cut -d ' ' -f 3)
        ((totRetrivedKB += retrivedKB))
    fi
done <"$input"
printf "Clients launched: %s\n" "${clients}"
printf "Total stores: %s\n" "${totStores}"
printf "Total retrives: %s\n" "${totRetrives}"
printf "Total deletes: %s\n" "${totDeletes}"
printf "Total successes: %s\n" "${totSuccess}"
printf "Total failures: %s\n" "${totFailures}"
printf "Total stored bytes: %s\n" "${totStoredKB}"
printf "Total retrived bytes: %s\n" "${totRetrivedKB}"

killall -SIGUSR1 server