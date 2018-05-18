//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include <thread>
#include <unistd.h> // for sleep, getpid
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);

uint64_t DelegateLoadClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, utils::Properties *props, const uint64_t num_ops, uint64_t &ops_so_far) {
  // ycsbc::CoreWorkload wl;
  // wl.InitLoad(*props);
  utils::Properties p=*props;
  if(p["dbname"]=="kvs_server") {
      db = ycsbc::DBFactory::CreateDB(p);
      if (!db) {
          cout << "Unknown database name " << p["dbname"] << endl;
          exit(0);
      }
  }
  db->Init();
  ycsbc::Client client(*db, *wl);
  uint64_t oks = 0;
  ops_so_far=0;
  for (uint64_t i = 0; i < num_ops; ++i) {
      oks += client.DoInsert();
      ops_so_far = oks;
  }
  db->Close();
  if(p["dbname"]=="kvs_server") {
      delete db;
  }
  return oks;
}

uint64_t DelegateRunClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, utils::Properties *props, const uint64_t num_ops, uint64_t &ops_so_far) {
  // ycsbc::CoreWorkload wl;
  // wl.InitRun(*props);
  utils::Properties p=*props;
  if(p["dbname"]=="kvs_server") {
      utils::Properties p=*props;
      db = ycsbc::DBFactory::CreateDB(p);
      if (!db) {
          cout << "Unknown database name " << p["dbname"] << endl;
          exit(0);
      }
  }
  db->Init();
  ycsbc::Client client(*db, *wl);
  uint64_t oks = 0;
  ops_so_far=0;
  for (uint64_t i = 0; i < num_ops; ++i) {
      oks += client.DoTransaction();
      ops_so_far = oks;
  }
  db->Close();
  if(p["dbname"]=="kvs_server") {
      delete db;
  }
  return oks;
}

uint64_t PrintStats(uint64_t const num_threads, uint64_t const total_target_ops, uint64_t *ops_so_far) {
    uint64_t prev_total=0;
    uint64_t cnt=0;
    uint64_t const interval=1;
    do {
        sleep(interval);
        uint64_t cur_total=0;
        for(uint64_t i=0; i<num_threads; i++) 
            cur_total+=ops_so_far[i];        
        std::cout << "# " << getpid() << " " << cnt << " Throughput (KTPS): " << (double)(cur_total-prev_total)/1000.0/(double)interval << std::endl;
        prev_total = cur_total;
        cnt++;
    }
    while(prev_total!=total_target_ops);
    return 0;
}

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db=NULL;
  if(props["dbname"]!="kvs_server") {
      db = ycsbc::DBFactory::CreateDB(props);
      if (!db) {
          cout << "Unknown database name " << props["dbname"] << endl;
          exit(0);
      }
  }

  std::cout << "Workload is " << file_name << std::endl;
  std::cout << "KVS type is " << props["dbname"] << std::endl;
  const uint64_t num_threads = stoul(props.GetProperty("threadcount", "1"));
  const int mode = stoi(props.GetProperty("mode", "0"));   // "mode" will be used by CreateDB

  vector<future<uint64_t>> actual_ops;
  if (mode != 2) {
      // not "run" only (2)
      cerr << "# Load phase " << endl;

      // Loads data
      uint64_t total_records = stoul(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
      uint64_t records_per_thread = total_records / num_threads;
      uint64_t remainder_records = total_records - records_per_thread*num_threads;
      uint64_t sum;

      ycsbc::CoreWorkload::InitLoadSharedState(props);
      ycsbc::CoreWorkload wl;
      wl.InitLoad(props);
      uint64_t *ops_so_far = new uint64_t[num_threads];

      utils::Timer<double> timer;
      timer.Start();
      for (uint64_t i = 0; i < num_threads; ++i) {
          if (i==num_threads-1) {
              records_per_thread += remainder_records;
          }
          actual_ops.emplace_back(async(launch::async,
                                        DelegateLoadClient, db, &wl, &props, records_per_thread, std::ref(ops_so_far[i])));
      }
      assert(actual_ops.size() == num_threads);

      sum=0;
      for (auto &n : actual_ops) {
          assert(n.valid());
          sum += n.get();
      }
      double duration = timer.End();

      cerr << "# Load total ops (load): " << sum << endl;
      assert(sum == total_records);
      cerr << "# Load total tp (KTPS): ";
      cerr << (double)sum / duration / 1000.0 << endl;
      cerr << "# Load thread tp (KTPS): ";
      cerr << (double)sum / duration / 1000.0/(double)num_threads << endl;
      cerr << "# Load avg latency (us): ";
      cerr << 0 << endl;
      cerr << "# Load thread latency (us): ";
      cerr << 0 << endl;

      actual_ops.clear();
      delete ops_so_far;
  }

  if (mode != 1) {
      // not "load" only (1)
      cerr << "# Run phase " << endl;

      // Peforms transactions
      uint64_t total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
      uint64_t ops_per_thread = total_ops / num_threads;
      uint64_t remainder_ops = total_ops - ops_per_thread*num_threads;
      uint64_t sum;

      ycsbc::CoreWorkload::InitRunSharedState(props);
      ycsbc::CoreWorkload wl;
      wl.InitRun(props);

      uint64_t *ops_so_far = new uint64_t[num_threads];
      
      utils::Timer<double> timer;
      timer.Start();
      for (uint64_t i = 0; i < num_threads; ++i) {
          if (i==num_threads-1) {
              ops_per_thread += remainder_ops;
          }
          actual_ops.emplace_back(async(launch::async,
                                        DelegateRunClient, db, &wl, &props, ops_per_thread, std::ref(ops_so_far[i])));
      }
      assert(actual_ops.size() == num_threads);

      /* for fault injection */
      //// stats thread start
      //future<uint64_t> stats(async(launch::async, PrintStats, num_threads, total_ops, ops_so_far));

      sum = 0;
      for (auto &n : actual_ops) {
          assert(n.valid());
          sum += n.get();
      }
      double duration = timer.End();

      /* for fault injection */
      //// stats thread end
      //stats.get();
      
      cerr << "# Run total ops (run): " << sum << endl;
      assert(sum == total_ops);

      cerr << "# Run total throughput (KTPS): ";
      cerr << (double)sum / duration / 1000.0 << endl;
      cerr << "# Run thread throughput (KTPS): ";
      cerr << (double)sum / duration / 1000.0/(double)num_threads << endl;
      cerr << "# Load avg latency (us): ";
      cerr << 0 << endl;
      cerr << "# Load thread latency (us): ";
      cerr << 0 << endl;

      actual_ops.clear();
      delete ops_so_far;
  }

  // make sure db is deleted before the static EpochManager instance is destroyed
  if(db)
      delete db;
}

string ParseCommandLine(int argc, const char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else if (strcmp(argv[argindex], "-mode") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      if (strcmp(argv[argindex], "load")==0) {
          props.SetProperty("mode", "1");
      }
      else if (strcmp(argv[argindex], "run")==0) {
          props.SetProperty("mode", "2");
      }
      else {
        UsageMessage(argv[0]);
        exit(0);
      }
      argindex++;
    } else if (strcmp(argv[argindex], "-location") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("location", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-cache_size") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("cache_size", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-nvmm_size") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("nvmm_size", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-nvmm_base") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("nvmm_base", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-nvmm_user") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("nvmm_user", argv[argindex]);
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
  cout << "  -mode modename: \"load\" - load data only; \"run\" - run benchmarks only (default: load and run)" << endl;
  cout << "  -location name: specify the location of the database, e.g., a root pointer" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}
