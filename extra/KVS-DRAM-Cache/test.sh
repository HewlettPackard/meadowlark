#!/bin/sh

cp nvmemul.ini.500 nvmemul.ini
bash test_script.sh 500 radix 1 read 1 full
bash test_script.sh 500 radix 1 read 2 full
bash test_script.sh 500 radix 1 read 4 full
bash test_script.sh 500 radix 1 read 8 full

bash test_script.sh 500 radix 1 update 1 full
bash test_script.sh 500 radix 1 update 2 full
bash test_script.sh 500 radix 1 update 4 full
bash test_script.sh 500 radix 1 update 8 full

bash test_script.sh 500 radix 1 insert 1 full
bash test_script.sh 500 radix 1 insert 2 full
bash test_script.sh 500 radix 1 insert 4 full
bash test_script.sh 500 radix 1 insert 8 full


bash test_script.sh 500 radix 1 read 1 short
bash test_script.sh 500 radix 1 read 2 short
bash test_script.sh 500 radix 1 read 4 short
bash test_script.sh 500 radix 1 read 8 short

bash test_script.sh 500 radix 1 update 1 short
bash test_script.sh 500 radix 1 update 2 short
bash test_script.sh 500 radix 1 update 4 short
bash test_script.sh 500 radix 1 update 8 short

bash test_script.sh 500 radix 1 insert 1 short
bash test_script.sh 500 radix 1 insert 2 short
bash test_script.sh 500 radix 1 insert 4 short
bash test_script.sh 500 radix 1 insert 8 short

cp nvmemul.ini.1000 nvmemul.ini
bash test_script.sh 1000 radix 1 read 1 full
bash test_script.sh 1000 radix 1 read 2 full
bash test_script.sh 1000 radix 1 read 4 full
bash test_script.sh 1000 radix 1 read 8 full

bash test_script.sh 1000 radix 1 update 1 full
bash test_script.sh 1000 radix 1 update 2 full
bash test_script.sh 1000 radix 1 update 4 full
bash test_script.sh 1000 radix 1 update 8 full

bash test_script.sh 1000 radix 1 insert 1 full
bash test_script.sh 1000 radix 1 insert 2 full
bash test_script.sh 1000 radix 1 insert 4 full
bash test_script.sh 1000 radix 1 insert 8 full


bash test_script.sh 1000 radix 1 read 1 short
bash test_script.sh 1000 radix 1 read 2 short
bash test_script.sh 1000 radix 1 read 4 short
bash test_script.sh 1000 radix 1 read 8 short

bash test_script.sh 1000 radix 1 update 1 short
bash test_script.sh 1000 radix 1 update 2 short
bash test_script.sh 1000 radix 1 update 4 short
bash test_script.sh 1000 radix 1 update 8 short

bash test_script.sh 1000 radix 1 insert 1 short
bash test_script.sh 1000 radix 1 insert 2 short
bash test_script.sh 1000 radix 1 insert 4 short
bash test_script.sh 1000 radix 1 insert 8 short
