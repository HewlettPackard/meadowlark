#include "cache_api.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <random>

#define NANO_SEC	1000000000
//1: Get
//2: Update
//3. Insert
#define TEST		1

struct str_arr{
	char *key;
	size_t key_len;
	char *val;
	size_t val_len;
};

//valiable
struct timespec *t = NULL;
struct str_arr *str_array = NULL;
uint64_t TOTAL_NUM = 1000000UL;
uint64_t NUM_THREADS = 1UL;
uint64_t INPUT_NUM = TOTAL_NUM / NUM_THREADS;

//Declared functions
int intN(int n);
char *randomString(int len);
void *do_work(void *arg);

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

void *do_work(void *arg)
{
	int ret;
	int *id = (int*)arg;
	uint64_t i, j, min, max;
	size_t val_len = 2048;
	char val[2048];

	min = INPUT_NUM * (*id);
	max = (INPUT_NUM * ((*id) + 1)) - 1;
#if TEST == 1
	for (i = 0, j = 0; i < INPUT_NUM; i++) {
		j = rand_uint64(0, TOTAL_NUM - 1);
		ret = Cache_Get(str_array[j].key, str_array[j].key_len, val, val_len);
		if (ret != 0) {
			printf("get error\n");
	//		exit(ret);
		}
		val_len = 2048;
		memset(val, 0, val_len);
	}
#elif TEST == 2
	for (i = 0, j = 0; i < INPUT_NUM; i++) {
		j = rand_uint64(0, TOTAL_NUM - 1);
		ret = Cache_Put(str_array[j].key, str_array[j].key_len, str_array[j].val, str_array[j].val_len);
		if (ret != 0) {
			printf("update error\n");
	//		exit(ret);
		}
	}
#elif TEST == 3
	for (i = min; i <= max; i++) {
		ret = Cache_Put(str_array[i].key, str_array[i].key_len, str_array[i].val, str_array[i].val_len);
		if (ret != 0) {
			printf("insert error\n");
	//		exit(ret);
		}
	}
#endif
	return NULL;
}

int main(int argc, char **argv)
{
	int ret;
	uint64_t i, key_range;
	size_t cache_size = atoi(argv[1]);
	int kvs_type = atoi(argv[2]);
	NUM_THREADS = (uint64_t) atoi(argv[3]);
	INPUT_NUM = (TOTAL_NUM / NUM_THREADS);
	unsigned long elapsed_time = 0;
	int *arg;

        size_t heap_size = 64*1024*1024*1024UL;
	KVS_Init(kvs_type, "", "" , "", heap_size, cache_size);

	pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t) * NUM_THREADS);
	t = (struct timespec *)calloc(1, sizeof(struct timespec) * 2);
	str_array = (struct str_arr *)calloc(1, sizeof(struct str_arr) * TOTAL_NUM);
	arg = (int *)calloc(1, sizeof(int) * NUM_THREADS);

	FILE *fp = fopen("./input_insert.txt", "r");
	for (i = 0; i < TOTAL_NUM; i++) {
		str_array[i].key = (char *)calloc(1, sizeof(char) * 100);
		str_array[i].val = (char *)calloc(1, sizeof(char) * 2048);
		fgets(str_array[i].key, 40, fp);
		fgets(str_array[i].val, 2048, fp);
		str_array[i].key_len = strlen(str_array[i].key);
		str_array[i].val_len = strlen(str_array[i].val);
	}
	fclose(fp);

#if TEST != 3
	//Insert
	for (i = 0; i < TOTAL_NUM; i++) {
		ret = Cache_Put(str_array[i].key, str_array[i].key_len, str_array[i].val, str_array[i].val_len);
		if (ret != 0) {
			printf("put error\n");
//			exit(ret);
		}
	}
	printf("Initilization is completed\n");
#endif

#if TEST == 2
	for (i = 0; i < TOTAL_NUM; i++)
		str_array[i].val_len = (str_array[i].val_len / 2);
#endif

	clock_gettime(CLOCK_MONOTONIC, &t[0]);
	for (i = 0; i < NUM_THREADS; i++) {
		arg[i] = i;
		pthread_create(&thread[i], NULL, do_work, &arg[i]);
	}

	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(thread[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t[1]);

	elapsed_time = (t[1].tv_sec - t[0].tv_sec) * NANO_SEC;
	elapsed_time += (t[1].tv_nsec - t[0].tv_nsec);

#if TEST == 1
	printf("=========get performance #%lu========\n", NUM_THREADS);
#elif TEST == 2
	printf("=========update performance #%lu========\n", NUM_THREADS);
#elif TEST == 3
	printf("=========insert performance #%lu========\n", NUM_THREADS);
#endif
	printf("ns = %lu\n", elapsed_time);
	printf("ms = %lu\n", elapsed_time * 1000 / NANO_SEC);
	printf("sec = %lu\n", elapsed_time / NANO_SEC);
	KVS_Final();

	free(t);
	free(str_array);
	free(arg);

	return 0;
}
