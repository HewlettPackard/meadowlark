//#include "memcached.h"
//#include "cache_api.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include <random>
#include <iostream>

#define TOTAL_NUM	1000000
#define NUM_THREADS	1
#define INPUT_NUM	(TOTAL_NUM / NUM_THREADS)

#define NANO_SEC	1000000000

struct str_arr{
	char *key;
	size_t key_len;
	char *val;
	size_t val_len;
};

struct timespec t1, t2;

//Declared functions
int intN(int n);
char *randomString(int len);
void *do_work1(void *arg);
void *do_work2(void *arg);

std::random_device r;
std::default_random_engine e1(r());
uint64_t rand_uint64(uint64_t min, uint64_t max)
{
	std::uniform_int_distribution<uint64_t> uniform_dist(min, max);
	return uniform_dist(e1);
}

const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";

int intN(int n) 
{ 
	return rand() % n;
}

char *randomString(int len) {
	char *rstr = (char *)malloc((len + 1) * sizeof(char));
	int i;
	for (i = 0; i < len; i++) {
		rstr[i] = alphabet[intN(strlen(alphabet))];
	}
	rstr[len] = '\0';
	return rstr;
}

void *do_work2(void *arg)
{
	int ret, i;
	struct str_arr *str_array = (struct str_arr *)malloc(INPUT_NUM * sizeof(struct str_arr));

	//libmemcached valiable
	memcached_server_st *servers = NULL;
	memcached_st *memc = NULL;
	memcached_return rc;

	memc = memcached_create(NULL);
	servers = memcached_server_list_append(servers, "localhost", 11211, &rc);
	rc = memcached_server_push(memc, servers);
	if (rc == MEMCACHED_SUCCESS)
		printf("server connection success\n");
	else
		printf("server connection fail\n");

	srand(time(NULL));
	for (i = 0; i < INPUT_NUM; i++) {
		//		sprintf(str_array[i].key, "%d", i);
		str_array[i].key = randomString(20);
		str_array[i].key_len = strlen(str_array[i].key);
		str_array[i].val = randomString(20);
		str_array[i].val_len = strlen(str_array[i].val);
		rc = memcached_set(memc, str_array[i].key, str_array[i].key_len, 
				str_array[i].val, str_array[i].val_len, 0, 0);
		if (rc == MEMCACHED_SUCCESS)
			continue;
		else
			printf("Put Fail\n");
	}

	char val[30];
	size_t val_len = 30;
	uint32_t flags;
	for (i = 0; i < INPUT_NUM; i++) {
		memcached_get(memc, str_array[i].key, str_array[i].key_len,
				&val_len, &flags, &rc);
		if (rc == MEMCACHED_SUCCESS)
			continue;
		else
			printf("Get: couldn't find key\n");
	}

	for (i = 0; i < INPUT_NUM/2; i++) {
		rc = memcached_delete(memc, str_array[i].key, 
				str_array[i].key_len, 0);
		if (rc == MEMCACHED_SUCCESS)
			continue;
		else
			printf("Del: couldn't delete key\n");
	}

	for (i = 0; i < INPUT_NUM/2; i ++) {
		memcached_get(memc, str_array[i].key, str_array[i].key_len,
				&val_len, &flags, &rc);
		if (rc == MEMCACHED_SUCCESS)
			printf("This key must not be found!\n");
		else
			continue;
	}
}


void *do_work1(void *arg)
{
	int ret, i;
	uint64_t *port = (uint64_t *) arg;
	unsigned long elapsed_time = 0;
	struct str_arr *str_array = (struct str_arr *)malloc(TOTAL_NUM * sizeof(struct str_arr));

	//libmemcached valiable
	memcached_server_st *servers = NULL;
	memcached_st *memc = NULL;
	memcached_return rc;

	memc = memcached_create(NULL);
	servers = memcached_server_list_append(servers, "localhost", *port, &rc);
	rc = memcached_server_push(memc, servers);
	if (rc == MEMCACHED_SUCCESS)
		printf("server connection success port num = %lu\n", *port);
	else
		printf("server connection fail port num = %lu\n", *port);

	for (i = 0; i < TOTAL_NUM; i++) {
		str_array[i].key = (char *)malloc(10);
		str_array[i].val = (char *)malloc(10);
		sprintf(str_array[i].key, "%d", i);
		sprintf(str_array[i].val, "%d", i);
		str_array[i].key_len = strlen(str_array[i].key);
		str_array[i].val_len = strlen(str_array[i].val);
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (i = 0; i < INPUT_NUM; i++) {
		rc = memcached_set(memc, str_array[i].key, str_array[i].key_len, 
				str_array[i].val, str_array[i].val_len, 0, 0);
		if (rc != MEMCACHED_SUCCESS) {
			std::cout << "Put fail: " << memcached_strerror(memc, rc) << std::endl;
			exit(0);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t2);

	elapsed_time = (t2.tv_sec - t1.tv_sec) * NANO_SEC;
	elapsed_time += (t2.tv_nsec - t1.tv_nsec);
	printf("=========put performance========\n");
	printf("ns = %lu\n", elapsed_time);
	printf("ms = %lu\n", elapsed_time * 1000 / NANO_SEC);
	printf("sec = %lu\n", elapsed_time / NANO_SEC);
/*
	char *return_val;
	size_t val_len = 2048;
	uint32_t flags;

	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (i = 0; i < INPUT_NUM; i++) {
		return_val = memcached_get(memc, str_array[i].key, str_array[i].key_len,
				&val_len, &flags, &rc);
		val_len = 2048;
		if (rc != MEMCACHED_SUCCESS) {
			std::cout << "Get fail: " << memcached_strerror(memc, rc) << std::endl;
			exit(0);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t2);
	elapsed_time = (t2.tv_sec - t1.tv_sec) * NANO_SEC;
	elapsed_time += (t2.tv_nsec - t1.tv_nsec);
	printf("=========get performance========\n");
	printf("ns = %lu\n", elapsed_time);
	printf("ms = %lu\n", elapsed_time * 1000 / NANO_SEC);
	printf("sec = %lu\n", elapsed_time / NANO_SEC);

	for (i = 0; i < INPUT_NUM/2; i++) {
		rc = memcached_delete(memc, str_array[i].key, 
				str_array[i].key_len, 0);
		if (rc != MEMCACHED_SUCCESS)
			printf("Del: couldn't delete key\n");
	}

	val_len = 2048;
	for (i = 0; i < INPUT_NUM/2; i++) {
		memcached_get(memc, str_array[i].key, str_array[i].key_len,
				&val_len, &flags, &rc);
		val_len = 2048;
		if (rc == MEMCACHED_SUCCESS)
			printf("This key must not be found!\n");
	}
	*/
}

int main(int argc, char **argv)
{
	int i;
	uint64_t port = atoi(argv[1]);

	pthread_t thread[NUM_THREADS];

	pthread_create(&thread[0], NULL, (void *(*)(void*))do_work1, &port);
//	pthread_create(&thread[0], NULL, (void *(*)(void*))do_work2, NULL);

	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(thread[i], NULL);

	return 0;
}
