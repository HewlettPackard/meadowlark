#!/bin/bash

WORKLOAD=$1

NODE01="l4tm@192.168.122.169"
NODE02="l4tm@192.168.122.171"
NODE03="l4tm@192.168.122.170"

COMMAND="ulimit -c unlimited; cd YCSB-C/build/bin; ./ycsbc -db kvs -threads 4 -P workloads/workload${WORKLOAD}.spec -mode load 2>&1; exit"

ssh ${NODE01} "${COMMAND}"


