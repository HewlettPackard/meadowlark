//
//  basic_db.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/17/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "db/db_factory.h"

#include <iostream>
#include <string>

#include <radixtree/kvs.h>

#include "db/dummy_db.h"
#include "db/basic_db.h"
#include "db/lock_stl_db.h"
// #include "db/tbb_rand_db.h"
// #include "db/tbb_scan_db.h"

#include "db/kvs_db.h"
#include "db/kvs_db_tiny.h"

#include "db/kvs_cache.h"
#include "db/kvs_cache_tiny.h"

#include "db/kvs_server.h"


using namespace std;
using ycsbc::DB;
using ycsbc::DBFactory;

DB* DBFactory::CreateDB(utils::Properties &props) {
  if (props["dbname"] == "kvs_dummy") {
    return new DummyDB;
  }
  else if (props["dbname"] == "basic") {
    return new BasicDB;
  } else if (props["dbname"] == "lock_stl") {
    return new LockStlDB;
  } 
  // else if (props["dbname"] == "tbb_rand") {
  //   return new TbbRandDB;
  // } 
  // else if (props["dbname"] == "tbb_scan") {
  //   return new TbbScanDB;
  // }
  // Server Mode
  // remember to re-build memcached with different CACHE and VERSION values
  else if (props["dbname"] == "kvs_server") {
      const std::string location = props.GetProperty("location", "");
      return new KVS_SERVER(location);
  }
  // Local Mode
  else {
      const std::string location = props.GetProperty("location", "");
      const std::string nvmm_base = props.GetProperty("nvmm_base", "");
      const std::string nvmm_user = props.GetProperty("nvmm_user", "");
      const size_t nvmm_size = stoul(props.GetProperty("nvmm_size", "1073741824"));
      const size_t cache_size = stoul(props.GetProperty("cache_size", "268435456"));
      
      // remember to re-build memcached with different CACHE and VERSION values
      if (props["dbname"] == "kvs_radixtree") {
          return new KVS_CACHE(KeyValueStore::RADIX_TREE, location, nvmm_base, nvmm_user, nvmm_size, cache_size);
      }
      else if (props["dbname"] == "kvs_radixtree_tiny") {
          return new KVS_CACHE_TINY(KeyValueStore::RADIX_TREE_TINY, location, nvmm_base, nvmm_user, nvmm_size, cache_size);
      }
      else if (props["dbname"] == "kvs_hashtable") {
          return new KVS_CACHE(KeyValueStore::HASH_TABLE, location, nvmm_base, nvmm_user, nvmm_size, cache_size);
      }
      else
          return NULL;
  }
}
