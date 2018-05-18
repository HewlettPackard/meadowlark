#ifndef YCSB_C_KVS_DB_H_
#define YCSB_C_KVS_DB_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include <thread>

#include "core/properties.h"

#include <radixtree/kvs.h>
#include <nvmm/global_ptr.h>

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

/*
  Local Mode, no cache
*/

using namespace radixtree;

namespace ycsbc {

class KVSDB : public DB {
 public:
    KVSDB(KeyValueStore::IndexType type, std::string root, std::string base, std::string user, size_t size):
        kvs_(nullptr) {
        //std::cout << "KVSDB() " << root << std::endl;
        nvmm::GlobalPtr root_gptr;
        root_gptr = str2gptr(root);
        if (root_gptr==0) {
            KeyValueStore::Reset(base, user);
        }
        KeyValueStore::Start(base, user);
        kvs_ = KeyValueStore::MakeKVS(type, root_gptr, base, user, size);
        assert(kvs_);
        std::cout << "KVS location is " << kvs_->Location() << std::endl;
    }

    ~KVSDB() {
        //std::cout << "~KVSDB() " << std::endl;
        delete kvs_;
    }

    nvmm::GlobalPtr str2gptr(std::string root_str) {
        std::string delimiter = ":";
        size_t loc = root_str.find(delimiter);
        if (loc==std::string::npos)
            return 0;
        std::string shelf_id = root_str.substr(1, loc-1);
        std::string offset = root_str.substr(loc+1, root_str.size()-3-shelf_id.size());
        return nvmm::GlobalPtr((unsigned char)std::stoul(shelf_id), std::stoul(offset));
    }

    int Read(const std::string &table, const std::string &key,
             const std::vector<std::string> *fields,
             std::vector<KVPair> &result) {
        //std::cout << "READ " << table << ' ' << key << std::endl;
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
            std::cout << "READ error " << ret << std::endl;
            return ret;
        }

        // decode value

        std::stringstream ss;
        ss.rdbuf()->pubsetbuf(val_buf, val_len);
        {
            cereal::BinaryInputArchive iarchive(ss);
            iarchive(result);
        }

        // char tmp[2048];
        // memcpy(tmp, val_buf, val_len);

        // for(auto pair: result) {
        //     std::cout << pair.first << " => " << pair.second << std::endl;
        // }

        return 0;
    }

    int Update(const std::string &table, const std::string &key,
               std::vector<KVPair> &values) {
        //std::cout << "UPDATE " << table << ' ' << key << std::endl;
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
        int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
                            val_buf.c_str(), val_buf.size());

        // char val_buf[2048];
        // char *val = val_buf;
        // for(auto &pair: values) {
        //     memcpy(val, pair.first.c_str(), pair.first.size());
        //     val+=pair.first.size();
        //     memcpy(val, pair.second.c_str(), pair.second.size());
        //     val+=pair.second.size();
        // }
        // size_t val_len = val-val_buf;

        // int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
        //                     val_buf, val_len);

        if(ret!=0)
            std::cout << "UPDATE error " << ret << std::endl;
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
        int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
                            val_buf.c_str(), val_buf.size());

        // char val_buf[2048];
        // char *val = val_buf;
        // for(auto &pair: values) {
        //     memcpy(val, pair.first.c_str(), pair.first.size());
        //     val+=pair.first.size();
        //     memcpy(val, pair.second.c_str(), pair.second.size());
        //     val+=pair.second.size();
        // }
        // size_t val_len = val-val_buf;
        // int ret = kvs_->Put(key_buf.c_str(), key_buf.size(),
        //                     val_buf, val_len);

        if(ret!=0)
            std::cout << "INSERT error " << ret << std::endl;
        return ret;
    }

    int Delete(const std::string &table, const std::string &key) {
        //std::cout << "DELETE " << table << ' ' << key << std::endl;

        // encode key
        std::string key_buf(table+key);
        assert(key_buf.size()<=kvs_->MaxKeyLen());
        int ret = kvs_->Del(key_buf.c_str(), key_buf.size());
        if(ret!=0)
            std::cout << "DELETE error " << ret << std::endl;
        return ret;
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

        // encode begin key
        std::string begin_key(table+key);
        assert(begin_key.size()<=kvs_->MaxKeyLen());

        // encode end key
        // TODO: same table?
        std::string end_key(KeyValueStore::OPEN_BOUNDARY_KEY, KeyValueStore::OPEN_BOUNDARY_KEY_SIZE);
        assert(end_key.size()<=kvs_->MaxKeyLen());

        // value buf
        static size_t const max_val_len = 4096;
        char val_buf[max_val_len];
        size_t val_len = max_val_len;

        // key buf
        static size_t const max_key_len = kvs_->MaxKeyLen();
        char key_buf[max_key_len];
        size_t key_len = max_key_len;

        // scan
        int iter;
        int ret = kvs_->Scan(iter,
                             key_buf, key_len,
                             val_buf, val_len,
                             begin_key.c_str(), begin_key.size(), true,
                             end_key.c_str(), end_key.size(), false);
        if(ret==0) {
            // decode value
            std::vector<KVPair> kvp;
            std::stringstream ss;
            ss.rdbuf()->pubsetbuf(val_buf, val_len);
            {
                cereal::BinaryInputArchive iarchive(ss);
                iarchive(kvp);
            }
            result.push_back(kvp);
            key_len = max_key_len;
            val_len = max_val_len;
        }
        else
            return ret; // TODO: return error?

        while(--len) {
            ret = kvs_->GetNext(iter,
                                key_buf, key_len,
                                val_buf, val_len);
            if (ret!=0)
                break;
            // decode value
            std::vector<KVPair> kvp;
            std::stringstream ss;
            ss.rdbuf()->pubsetbuf(val_buf, val_len);
            {
                cereal::BinaryInputArchive iarchive(ss);
                iarchive(kvp);
            }
            result.push_back(kvp);
            key_len = max_key_len;
            val_len = max_val_len;
        }

        //std::cout << "# of records: " << result.size() << " (" << len << ") " << std::endl;
        // for(auto record: result) {
        //     for(auto pair: record) {
        //         std::cout << pair.first << " => " << pair.second << std::endl;
        //     }
        // }
        return 0;
    }

private:
    KeyValueStore *kvs_;

};

} // ycsbc

#endif // YCSB_C_KVS_DB_H_
