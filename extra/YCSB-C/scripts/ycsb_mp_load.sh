#!/bin/bash

WORKLOAD=$1
KVS=$2
THREAD=$3

COMMAND="ulimit -c unlimited; ${YCSB_PATH}/ycsbc -db ${KVS} -threads ${THREAD} -P ${YCSB_PATH}/workloads/${WORKLOAD}.spec -mode load 2>&1; cd ~"

eval $COMMAND
