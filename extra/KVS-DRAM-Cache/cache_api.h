#ifndef CACHE_API_H
#define CACHE_API_H
extern "C"{
#include "memcached.h"
}
#include <string>
#include <radixtree/kvs.h>
#include <cluster/config.h>
#include <nvmm/memory_manager.h>

extern "C" item *item_get(const char *key, const size_t nkey, conn *c, const bool do_update);
extern "C" enum store_item_type store_item(item *item, int comm, conn* c);
extern "C" void item_remove(item *item);
extern "C" void item_unlink(item *item);
extern "C" int item_unlink_kvs(const char *key, const size_t nkey);

// Local Mode
// it calls Cache_Final()
void KVS_Final();

// Local Mode
// called from application
// it calls Cache_Init()
void KVS_Init(radixtree::KeyValueStore::IndexType type, std::string root="", std::string base="", std::string user="",
              size_t size=64*1024*1024*1024UL, size_t cache_size=0);

void KVS_Init(std::string type, std::string root="", std::string base="", std::string user="",
              size_t size=64*1024*1024*1024UL, size_t cache_size=0);

// Cluster/Server Mode
// called from server frontend, which already initialized cache
// it does not call Cache_Init()
void KVS_Init(radixtree::KVS kvs_config);

// Local Mode
int Cache_Put(char const *key, size_t const key_len, char const *val, size_t const val_len);
int Cache_Get(char const *key, size_t const key_len, char *val, size_t &val_len);
int Cache_Del(char const *key, size_t const key_len);

// // Local Mode: cache only
// // requires CACHE==5
// int mc_Put(char const *key, size_t const key_len, char const *val, size_t const val_len);
// int mc_Get(char const *key, size_t const key_len, char *val, size_t &val_len);
// int mc_Del(char const *key, size_t const key_len);

#endif
