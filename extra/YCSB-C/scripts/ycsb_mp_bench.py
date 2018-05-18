#!/usr/bin/python

from sys import argv
import os
import subprocess

if os.environ.get("YCSB_PATH") is None:
    print "Please set the environment variable YCSB_PATH to the path to ycsbc binary"
    sys.exit(1)

kvs = argv[1]
workload = argv[2]
thread = int(argv[3])
process = int(argv[4])
total = int(argv[5])

sum=0.0

if kvs == "dummy":
    for i in range(0,total):
        # load data and run benchmark
        run = "bash ycsb_mp.sh %s %s %s %s | grep %s | cut -f4 | awk '{s+=$1} END {print s}'" % (workload, kvs, thread, process, workload)
        result = subprocess.check_output(run, shell=True).strip()
        tp = float(result.strip())

        sum+=tp

else:
    for i in range(0,total):
        # load data
        load = "bash ycsb_mp_load.sh %s %s 4 | grep location | awk '{print $4}'" % (workload, kvs)
        result = subprocess.check_output(load, shell=True)
        location = result.strip()

        # run benchmark
        run = "bash ycsb_mp_run.sh %s %s %s %s %s | grep %s | cut -f4 | awk '{s+=$1} END {print s}'" % (workload, kvs, thread, process, location, workload)
        result = subprocess.check_output(run, shell=True).strip()
        tp = float(result.strip())

        sum+=tp

print "avg tp of %s for %s over %s runs is %s" % (kvs, workload, total, sum/total)
