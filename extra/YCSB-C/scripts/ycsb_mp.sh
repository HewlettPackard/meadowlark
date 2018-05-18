#!/bin/bash

WORKLOAD=$1
KVS=$2
THREAD=$3
PROCESS=$4

for (( i=0; i<${PROCESS}; i++ ))
do
    COMMAND="ulimit -c unlimited; ${YCSB_PATH}/ycsbc -db ${KVS} -threads ${THREAD} -P ${YCSB_PATH}/workloads/${WORKLOAD}.spec -mode run 2>&1"
    eval ${COMMAND} &
done
