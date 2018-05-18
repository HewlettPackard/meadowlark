#!/bin/sh

APP_OUT() {
	echo -e $*
}

LATENCY=$1
WORKLOAD=$2
DISTRIBUTION=$3

APP_OUT() {
	echo -e $*
}

APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},4" &>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"kvs,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/kvs/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"default,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/default/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"full-p,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/full-p/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"full-v,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/full-v/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"hybrid-p,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/hybrid-p/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"hybrid-v,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/hybrid-v/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"short,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n\n",avg;}' \
	./results/${LATENCY}/short/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv

APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},4" &>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"kvs,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/kvs/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"default,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/default/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"full-p,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/full-p/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"full-v,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/full-v/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"hybrid-p,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/hybrid-p/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"hybrid-v,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
	./results/${LATENCY}/hybrid-v/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv
awk 'BEGIN{avg=0; num=0; printf"short,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n\n",avg;}' \
	./results/${LATENCY}/short/${DISTRIBUTION}/4/${WORKLOAD}_512.txt \
	&>> ./results.csv


##APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},2,4" &>> ./results.csv
##awk 'BEGIN{avg=0; num=0; printf"kvs,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/kvs/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
##	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/kvs/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/kvs/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
##	&>> ./results.csv
#
#APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},4" &>> ./results.csv
#awk 'BEGIN{avg=0; num=0; printf"full,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
#	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/full/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
##	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"short,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/short/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/short/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
#	./results/${LATENCY}/short/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##
##awk 'BEGIN{avg=0; num=0; printf"radixtree,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/radixtree/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
##	&>> ./results.csv
###awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
###	./results/${LATENCY}/radixtree/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
###	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/radixtree/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
##	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"hybrid,";} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/hybrid/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /workload/{avg+=$4; num++;} END{avg=(avg/num); printf"%f\n\n",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
#	&>> ./results.csv
#
#
#
##APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},2,4" &>> ./results.csv
##awk 'BEGIN{avg=0; num=0; printf"kvs,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/kvs/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
##	&>> ./results.csv
###awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
###	./results/${LATENCY}/kvs/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
###	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/kvs/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
##	&>> ./results.csv
#
#APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},2,4" &>> ./results.csv
#awk 'BEGIN{avg=0; num=0; printf"full,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/full/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
#	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"short,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/short/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/short/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
#	./results/${LATENCY}/short/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##
##awk 'BEGIN{avg=0; num=0; printf"radixtree,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/radixtree/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
##	&>> ./results.csv
###awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
###	./results/${LATENCY}/radixtree/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
###	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/radixtree/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
##	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"hybrid,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/2/${WORKLOAD}_64.txt \
#	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/hybrid/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n\n",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/4/${WORKLOAD}_64.txt \
#	&>> ./results.csv



#APP_OUT "Run_workload${WORKLOAD}_${DISTRIBUTION},1,2,4" &>> ./results.csv
#awk 'BEGIN{avg=0; num=0; printf"full,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/1/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
#	./results/${LATENCY}/full/${DISTRIBUTION}/4/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#
##awk 'BEGIN{avg=0; num=0; printf"short,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/short/${DISTRIBUTION}/1/${WORKLOAD}_4.txt \
##	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
##	./results/${LATENCY}/short/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
##	&>> ./results.csv
##awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
##	./results/${LATENCY}/short/${DISTRIBUTION}/4/${WORKLOAD}_4.txt \
##	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"radixtree,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/radixtree/${DISTRIBUTION}/1/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/radixtree/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n",avg;}' \
#	./results/${LATENCY}/radixtree/${DISTRIBUTION}/4/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#
#awk 'BEGIN{avg=0; num=0; printf"hybrid,";} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/1/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f,",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/2/${WORKLOAD}_4.txt \
#	&>> ./results.csv
#awk 'BEGIN{avg=0; num=0;} /hit/{avg+=$3; num++;} END{avg=(avg/num); printf"%f\n\n\n",avg;}' \
#	./results/${LATENCY}/hybrid/${DISTRIBUTION}/4/${WORKLOAD}_4.txt \
#	&>> ./results.csv
