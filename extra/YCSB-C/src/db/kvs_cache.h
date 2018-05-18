#ifndef YCSB_C_KVS_CACHE_H_
#define YCSB_C_KVS_CACHE_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include <thread>

#include "core/properties.h"

#include <radixtree/kvs.h>
#include <nvmm/global_ptr.h>
#include <cache_api.h> // kvs cache API

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>


/*
  Local Mode, with cache
*/

using namespace radixtree;


namespace ycsbc {

class KVS_CACHE : public DB {
 public:
    KVS_CACHE(KeyValueStore::IndexType type, std::string root, std::string base, std::string user, size_t size, size_t cache_size) {
        //std::cout << "KVS_CACHE() " << root << std::endl;
        KVS_Init(type, root, base, user, size, cache_size);
    }

    ~KVS_CACHE() {
        //std::cout << "~KVS_CACHE() " << std::endl;
        KVS_Final();
    }

    int Read(const std::string &table, const std::string &key,
             const std::vector<std::string> *fields,
             std::vector<KVPair> &result) {
        // std::cout << "READ " << table << ' ' << key << std::endl;
        // if (fields) {
        //     std::cout << " [ ";
        //     for (auto f : *fields) {
        //         std::cout << f << ' ';
        //     }
        //     std::cout << ']' << std::endl;
        // } else {
        //     std::cout  << " < all fields >" << std::endl;
        // }

        // encode key
        std::string key_buf(table+key);
        //assert(key_buf.size()<=kvs_->MaxKeyLen());

        static size_t const max_val_len = 4096;
        char val_buf[max_val_len];
        size_t val_len = max_val_len;
        int ret = Cache_Get(key_buf.c_str(), key_buf.size(),
                            val_buf, val_len);

        if(ret!=0) {
            std::cout << "READ error " << std::endl;
            return ret;
        }

        // decode value
        std::stringstream ss;
        ss.rdbuf()->pubsetbuf(val_buf, val_len);
        {
            cereal::BinaryInputArchive iarchive(ss);
            iarchive(result);
        }

        // for(auto pair: result) {
        //     std::cout << pair.first << " => " << pair.second << std::endl;
        // }

        return 0;
    }

    int Scan(const std::string &table, const std::string &key,
             int len, const std::vector<std::string> *fields,
             std::vector<std::vector<KVPair>> &result) {
        // std::cout << " SCAN " << table << ' ' << key << " ("<< len << ")" << std::endl;
        // if (fields) {
        //     std::cout << " [ ";
        //     for (auto f : *fields) {
        //         std::cout << f << ' ';
        //     }
        //     std::cout << ']' << std::endl;
        // } else {
        //     std::cout  << " < all fields >" << std::endl;
        // }

        // // encode begin key
        // std::string begin_key(table+key);
        // assert(begin_key.size()<=kvs_->MaxKeyLen());

        // // encode end key
        // // TODO: same table?
        // std::string end_key(KeyValueStore::OPEN_BOUNDARY_KEY, KeyValueStore::OPEN_BOUNDARY_KEY_SIZE);
        // assert(end_key.size()<=kvs_->MaxKeyLen());

        // // value buf
        // static size_t const max_val_len = 4096;
        // char val_buf[max_val_len];
        // size_t val_len = max_val_len;

        // // key buf
        // static size_t const max_key_len = kvs_->MaxKeyLen();
        // char key_buf[max_key_len];
        // size_t key_len = max_key_len;

        // // scan
        // int iter;
        // int ret = kvs_->Scan(iter,
        //                      key_buf, key_len,
        //                      val_buf, val_len,
        //                      begin_key.c_str(), begin_key.size(), true,
        //                      end_key.c_str(), end_key.size(), false);
        // if(ret==0) {
        //     // decode value
        //     std::vector<KVPair> kvp;
        //     std::stringstream ss;
        //     ss.rdbuf()->pubsetbuf(val_buf, val_len);
        //     {
        //         cereal::BinaryInputArchive iarchive(ss);
        //         iarchive(kvp);
        //     }
        //     result.push_back(kvp);
        //     key_len = max_key_len;
        //     val_len = max_val_len;
        // }
        // else
        //     return 0; // TODO: return error?

        // while(--len) {
        //     ret = kvs_->GetNext(iter,
        //                         key_buf, key_len,
        //                         val_buf, val_len);
        //     if (ret!=0)
        //         break;
        //     // decode value
        //     std::vector<KVPair> kvp;
        //     std::stringstream ss;
        //     ss.rdbuf()->pubsetbuf(val_buf, val_len);
        //     {
        //         cereal::BinaryInputArchive iarchive(ss);
        //         iarchive(kvp);
        //     }
        //     result.push_back(kvp);
        //     key_len = max_key_len;
        //     val_len = max_val_len;
        // }

        //std::cout << "# of records: " << result.size() << " (" << len << ") " << std::endl;
        // for(auto record: result) {
        //     for(auto pair: record) {
        //         std::cout << pair.first << " => " << pair.second << std::endl;
        //     }
        // }
        return 0;
    }

    int Update(const std::string &table, const std::string &key,
               std::vector<KVPair> &values) {
        // std::cout << "UPDATE " << table << ' ' << key << std::endl;
        // for(auto pair: values) {
        //     std::cout << pair.first << " => " << pair.second << std::endl;
        // }

        // encode key
        std::string key_buf(table+key);
        //assert(key_buf.size()<=kvs_->MaxKeyLen());

        // encode value
        std::stringstream ss;
        {
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            oarchive(values);
        }
        std::string val_buf = ss.str();
        int ret = Cache_Put(key_buf.c_str(), key_buf.size(),
                            val_buf.c_str(), val_buf.size());
        if(ret!=0)
            std::cout << "UPDATE error " << std::endl;
        return ret;
    }

    int Insert(const std::string &table, const std::string &key,
               std::vector<KVPair> &values) {
        //std::cout << "INSERT " << table << ' ' << key << std::endl;
        // for(auto pair: values) {
        //     std::cout << pair.first << " => " << pair.second << std::endl;
        // }

        // encode key
        std::string key_buf(table+key);
        //assert(key_buf.size()<=kvs_->MaxKeyLen());

        // encode value
        std::stringstream ss; // any stream can be used
        {
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            oarchive(values);
        }
        std::string val_buf = ss.str();
        //std::string val_buf = "value";
        int ret = Cache_Put(key_buf.c_str(), key_buf.size(),
                            val_buf.c_str(), val_buf.size());
        if(ret!=0)
            std::cout << "INSERT error " << std::endl;
        return ret;
    }

    int Delete(const std::string &table, const std::string &key) {
        // std::cout << "DELETE " << table << ' ' << key << std::endl;

        // encode key
        std::string key_buf(table+key);
        //assert(key_buf.size()<=kvs_->MaxKeyLen());
        int ret = Cache_Del(key_buf.c_str(), key_buf.size());
        if(ret!=0)
            std::cout << "DELETE error " << std::endl;
        return ret;
    }

};

} // ycsbc

#endif // YCSB_C_KVS_CACHE_H_
