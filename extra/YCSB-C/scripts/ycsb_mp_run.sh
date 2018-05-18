#!/bin/bash

WORKLOAD=$1
KVS=$2
THREAD=$3
PROCESS=$4
LOCATION=$5

COMMAND="ulimit -c unlimited; ${YCSB_PATH}/ycsbc -db ${KVS} -threads ${THREAD} -P ${YCSB_PATH}/workloads/${WORKLOAD}.spec -mode run -location ${LOCATION} 2>&1; cd ~"
for (( i=0; i<${PROCESS}; i++ ))
do
    eval ${COMMAND} &
done
