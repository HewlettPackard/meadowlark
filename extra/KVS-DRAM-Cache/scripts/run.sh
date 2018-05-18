#!/bin/sh

#bash test_cache.sh 1000 hybrid-p c zipfian 4 512 512
#bash test_cache.sh 1000 hybrid-v c zipfian 4 512 512
#bash test_cache.sh 1000 full-p c zipfian 4 512 512
#bash test_cache.sh 1000 full-v c zipfian 4 512 512
#bash test_cache.sh 1000 short c zipfian 4 512 512
#bash test_cache.sh 1000 default c zipfian 4 512 512
#bash test_kvs.sh 1000 kvs c zipfian 4 512 549755813888
#
#bash test_cache.sh 1000 hybrid-p b zipfian 4 512 512
bash test_cache.sh 1000 hybrid-v b zipfian 4 512 512
bash test_cache.sh 1000 full-p b zipfian 4 512 512
bash test_cache.sh 1000 full-v b zipfian 4 512 512
bash test_cache.sh 1000 short b zipfian 4 512 512
bash test_cache.sh 1000 default b zipfian 4 512 512
bash test_kvs.sh 1000 kvs b zipfian 4 512 549755813888


bash test_cache.sh 1000 hybrid-p a zipfian 4 512 512
bash test_cache.sh 1000 hybrid-v a zipfian 4 512 512
bash test_cache.sh 1000 full-p a zipfian 4 512 512
bash test_cache.sh 1000 full-v a zipfian 4 512 512
bash test_cache.sh 1000 short a zipfian 4 512 512
bash test_cache.sh 1000 default a zipfian 4 512 512
bash test_kvs.sh 1000 kvs a zipfian 4 512 549755813888


rm /tmp/nvm0/*
cp /dev/shm/volos/* /tmp/nvm0/
cp workloads/workloadc_1000000 workloads/workloadc.spec
bash test_cache.sh 1000 hybrid-p c 1000000 4 512 512
bash test_cache.sh 1000 hybrid-v c 1000000 4 512 512
bash test_cache.sh 1000 full-p c 1000000 4 512 512
bash test_cache.sh 1000 full-v c 1000000 4 512 512
bash test_cache.sh 1000 short c 1000000 4 512 512
bash test_cache.sh 1000 default c 1000000 4 512 512
bash test_kvs.sh 1000 kvs c 1000000 4 512 549755813888

cp workloads/workloadc_2000000 workloads/workloadc.spec
bash test_cache.sh 1000 hybrid-p c 2000000 4 512 512
bash test_cache.sh 1000 hybrid-v c 2000000 4 512 512
bash test_cache.sh 1000 full-p c 2000000 4 512 512
bash test_cache.sh 1000 full-v c 2000000 4 512 512
bash test_cache.sh 1000 short c 2000000 4 512 512
bash test_cache.sh 1000 default c 2000000 4 512 512
bash test_kvs.sh 1000 kvs c 2000000 4 512 549755813888


cp workloads/workloada_1000000 workloads/workloada.spec
bash test_cache.sh 1000 hybrid-p a 1000000 4 512 512
bash test_cache.sh 1000 hybrid-v a 1000000 4 512 512
bash test_cache.sh 1000 full-p a 1000000 4 512 512
bash test_cache.sh 1000 full-v a 1000000 4 512 512
bash test_cache.sh 1000 short a 1000000 4 512 512
bash test_cache.sh 1000 default a 1000000 4 512 512
bash test_kvs.sh 1000 kvs a 1000000 4 512 549755813888


cp workloads/workloada_2000000 workloads/workloada.spec
bash test_cache.sh 1000 hybrid-p a 2000000 4 512 512
bash test_cache.sh 1000 hybrid-v a 2000000 4 512 512
bash test_cache.sh 1000 full-p a 2000000 4 512 512
bash test_cache.sh 1000 full-v a 2000000 4 512 512
bash test_cache.sh 1000 short a 2000000 4 512 512
bash test_cache.sh 1000 default a 2000000 4 512 512
bash test_kvs.sh 1000 kvs a 2000000 4 512 549755813888


cp workloads/workloadb_1000000 workloads/workloadb.spec
bash test_cache.sh 1000 hybrid-p b 1000000 4 512 512
bash test_cache.sh 1000 hybrid-v b 1000000 4 512 512
bash test_cache.sh 1000 full-p b 1000000 4 512 512
bash test_cache.sh 1000 full-v b 1000000 4 512 512
bash test_cache.sh 1000 short b 1000000 4 512 512
bash test_cache.sh 1000 default b 1000000 4 512 512
bash test_kvs.sh 1000 kvs b 1000000 4 512 549755813888

cp workloads/workloadb_2000000 workloads/workloadb.spec
bash test_cache.sh 1000 hybrid-p b 2000000 4 512 512
bash test_cache.sh 1000 hybrid-v b 2000000 4 512 512
bash test_cache.sh 1000 full-p b 2000000 4 512 512
bash test_cache.sh 1000 full-v b 2000000 4 512 512
bash test_cache.sh 1000 short b 2000000 4 512 512
bash test_cache.sh 1000 default b 2000000 4 512 512
bash test_kvs.sh 1000 kvs b 2000000 4 512 549755813888
