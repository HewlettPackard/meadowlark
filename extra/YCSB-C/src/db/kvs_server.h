#ifndef YCSB_C_KVS_SERVER_H_
#define YCSB_C_KVS_SERVER_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include <thread>

#include "core/properties.h"

#include <kvs_client/kvs_client.h> // client lib to use kvs server

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

/*
  Server Mode
*/

using namespace radixtree;

namespace ycsbc {

class KVS_SERVER : public DB {
 public:
    KVS_SERVER(std::string config_file):
        kvs_(nullptr) {
        //std::cout << "KVS_SERVER() " << root << std::endl;
        assert(!config_file.empty());
        kvs_ = new KVSServer();
        assert(kvs_);
        int ret = kvs_->Init(config_file);
        assert(ret==0);
        //kvs_->PrintCluster();
    }

    ~KVS_SERVER() {
        //std::cout << "~KVS_SERVER() " << std::endl;
        if(kvs_) {
            kvs_->Final();
            delete kvs_;
        }
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
        assert(key_buf.size()<=kvs_->MaxKeyLen());

        static size_t const max_val_len = 4096;
        char val_buf[max_val_len];
        size_t val_len = max_val_len;
        int ret = kvs_->Get(key_buf.c_str(), key_buf.size(),
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

    int Update(const std::string &table, const std::string &key,
               std::vector<KVPair> &values) {
        // std::cout << "UPDATE " << table << ' ' << key << std::endl;
        // for(auto pair: values) {
        //     std::cout << pair.first << " => " << pair.second << std::endl;
        // }

        // encode key
        std::string key_buf(table+key);
        assert(key_buf.size()<=kvs_->MaxKeyLen());

        // encode value
        std::stringstream ss;
        {
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            oarchive(values);
        }
        std::string val_buf = ss.str();
        //std::string val_buf = "value";
        int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
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
        assert(key_buf.size()<=kvs_->MaxKeyLen());

        // encode value
        std::stringstream ss; // any stream can be used
        {
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            oarchive(values);
        }
        std::string val_buf = ss.str();
        //std::string val_buf = "value";
        int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
                            val_buf.c_str(), val_buf.size());
        if(ret!=0)
            std::cout << "INSERT error " << std::endl;
        return ret;
    }

    int Delete(const std::string &table, const std::string &key) {
        // std::cout << "DELETE " << table << ' ' << key << std::endl;

        // encode key
        std::string key_buf(table+key);
        assert(key_buf.size()<=kvs_->MaxKeyLen());
        int ret = kvs_->Del(key_buf.c_str(), key_buf.size());
        if(ret!=0)
            std::cout << "DELETE error " << std::endl;
        return ret;
    }

    int Scan(const std::string &table, const std::string &key,
             int len, const std::vector<std::string> *fields,
             std::vector<std::vector<KVPair>> &result) {
        std::cout << "Scan is not supported" << std::endl;
        return -1;
    }

private:
    KVSServer *kvs_;

};

} // ycsbc

#endif // YCSB_C_KVS_SERVER_H_
