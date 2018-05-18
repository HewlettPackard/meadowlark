#ifndef YCSB_C_DUMMY_DB_H_
#define YCSB_C_DUMMY_DB_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include "core/properties.h"

using std::cout;
using std::endl;

namespace ycsbc {

class DummyDB : public DB {
 public:
  void Init() {
      //std::lock_guard<std::mutex> lock(mutex_);
      //cout << "A new thread begins working." << endl;
  }

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result) {
    return 0;
  }

  int Scan(const std::string &table, const std::string &key,
           int len, const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) {
    return 0;
  }

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return 0;
  }

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return 0;
  }

  int Delete(const std::string &table, const std::string &key) {
    return 0;
  }

 private:
    //std::mutex mutex_;
};

} // ycsbc

#endif // YCSB_C_DUMMY_DB_H_
