#!/bin/sh

##These parameters can be changed from directory path for results files
##Please refer to ./results directory and run.sh
LATENCY=$1
CACHE=$2
WORKLOAD=$3
SKEW=$4
THREAD=$5
#The granularity of size parameters is bytes
CACHE_SIZE=$6
FAM_SIZE=$7

FAM_PATH="/tmp/nvm0"

QUARTZ_SO=/home/leesek/projects/quartz/build/src/lib/libnvmemul.so
OUTPUT=./results/${LATENCY}/${CACHE}/${SKEW}/${THREAD}


TIMES=6

APP_OUT() {
	echo -e $*
}

#mkdir /dev/shm/leesek

#SIZE=32
#for i in $(eval "echo {1..${TIMES}}"); do
#	SIZE=`expr 2 \* ${SIZE}`
#	APP_OUT "workload${WORKLOAD}_${SIZE}_${LATENCY}_${CACHE} Iteration ${i}"
#	rm -rf ${FAM_PATH}/*
#	LD_PRELOAD=${QUARTZ_SO} ./ycsbc -db kvs_cache_${CACHE} -threads ${THREAD} \
#		-P workloads/workload${WORKLOAD}.spec \
#		-nvmm_base ${FAM_PATH} -nvmm_size ${SIZE} \
#		&>> ${OUTPUT}/${WORKLOAD}_${SIZE}.txt 
#	APP_OUT "\n\n" &>> ${OUTPUT}/${WORKLOAD}_${SIZE}.txt
#done
#
#SIZE=32
#for i in $(eval "echo {1..${TIMES}}"); do
#	SIZE=`expr 2 \* ${SIZE}`
#	APP_OUT "workload${WORKLOAD}_${SIZE}_${LATENCY}_${CACHE} Iteration ${i}"
#	rm -rf ${FAM_PATH}/*
#	./ycsbc -db kvs_cache_${CACHE} -threads ${THREAD} \
#		-P workloads/workload${WORKLOAD}.spec \
#		-nvmm_base ${FAM_PATH} -nvmm_size ${SIZE} \
#		&>> ${OUTPUT}/${WORKLOAD}_${SIZE}.txt 
#	APP_OUT "\n\n" &>> ${OUTPUT}/${WORKLOAD}_${SIZE}.txt
#done

APP_OUT "workload${WORKLOAD}_${LATENCY}_${CACHE}_${THREAD}"
APP_OUT "First Iter"
if [[ ${WORKLOAD} != "c" ]]; then
	rm /tmp/nvm0/*
	cp /dev/shm/volos/* /tmp/nvm0/
fi
LD_PRELOAD=${QUARTZ_SO} ./ycsbc-${CACHE} -db kvs_cache_radixtree -threads ${THREAD} \
	-P workloads/workload${WORKLOAD}.spec -nvmm_base ${FAM_PATH} \
	-nvmm_size ${FAM_SIZE} -cache_size ${CACHE_SIZE} -mode run -location [33:4096] \
	&>> ${OUTPUT}/${WORKLOAD}_${CACHE_SIZE}.txt 
APP_OUT "\n\n" &>> ${OUTPUT}/${WORKLOAD}_${CACHE_SIZE}.txt
APP_OUT "Second Iter"
if [[ ${WORKLOAD} != "c" ]]; then
	rm /tmp/nvm0/*
	cp /dev/shm/volos/* /tmp/nvm0/
fi
LD_PRELOAD=${QUARTZ_SO} ./ycsbc-${CACHE} -db kvs_cache_radixtree -threads ${THREAD} \
	-P workloads/workload${WORKLOAD}.spec -nvmm_base ${FAM_PATH} \
	-nvmm_size ${FAM_SIZE} -cache_size ${CACHE_SIZE} -mode run -location [33:4096] \
	&>> ${OUTPUT}/${WORKLOAD}_${CACHE_SIZE}.txt 
APP_OUT "\n\n" &>> ${OUTPUT}/${WORKLOAD}_${CACHE_SIZE}.txt
