# YCSB-C

Yahoo! Cloud Serving Benchmark in C++, a C++ version of
[YCSB](https://github.com/brianfrankcooper/YCSB), based on [YCSB-C](https://github.com/basicthinker/YCSB-C)

## Dependencies

- Install intel tbb library

  ```
  $ sudo apt-get install libtbb-dev
  ```

  Redhat systems

  ```
  $ sudo apt-get install tbb-devel
  ```

- Install [NVMM](https://github.hpe.com/labs/nvmm) and [RadixTree](https://github.hpe.com/labs/radixtree)

## Build & Run

1. Download the source code

   ```
   $ git clone git@github.hpe.com:labs/YCSB-C.git
   ```

2. Point build system to NVMM and RadixTree   

   Set the environment variable ${CMAKE_PREFIX_PATH} to include the paths to the header and library
   files of NVMM and RadixTree. 

   For example, if NVMM and RadixTree are installed at /home/${USER}/projects/nvmm and /home/${USER}/projects/radixtree:

   ```
   $ export CMAKE_PREFIX_PATH=/home/${USER}/projects/nvmm/include:/home/${USER}/projects/nvmm/build/src:/home/${USER}/projects/radixtree/include:/home/${USER}/projects/radixtree/build/src:/home/${USER}/projects/memcached
   ```

3. Build

  On CC-NUMA systems (default):

  ```
  $ mkdir build
  $ cd build
  $ cmake .. -DFAME=OFF
  $ make
  ```

  On FAME:

  ```
  $ mkdir build
  $ cd build
  $ cmake .. -DFAME=ON
  $ make
  ```


4. Run (single-node)

   ```
   $ cd bin
   $ ./ycsbc -db TYPE -threads CNT -P workloads/WORKLOAD.spec [-mode MODE] [-location LOC] [-nvmm_base NVMM_PATH] [-nvmm_size NVMM_SIZE] [-cache_size CACHE_SIZE]
   ```

   TYPE is the type of backend key-value store, which currently can be basic, lock_stl, tbb_rand,
   tbb_scan, kvs_radixtree, kvs_hashtable, kvs_cache_radixtree, kvs_cache_hashtable, or kvs_server. Note that kvs_* is our key-value store implementation
   (see src/db/kvs_db.h and src/db/kvs_server.h). 

   CNT is the thread/client count of the benchmark. 

   WORKLOAD is the filename of the workload you want to run. Currently there are 6 benchmarks from
   YCSB (see bin/workloads). 

   Other options are optional. 

   MODE can be "load" (load data only) or "run" (run benchmark only). LOC is the location (root
   pointer) of the key value store data structure for kvs in local library mode (kvs_radixtree or
   kvs_hashtable), but for kvs in server mode (kvs_server) it is the path to the cluster config
   file. Both options are used in FAME to have one node create the key
   value store (printing out LOC) and preload data entirely (MODE="load"), before starting multiple
   ycsbc instances from multiple nodes to run the benchmark base on the same key value store
   location (LOC). 

   There are two optinos to config NVMM for kvs in local library mode, NVMM_PATH is the path of the
   directory that stores shelves. NVMM_SIZE is the capacity of the heap that will be used by
   kvs. For kvs running in server mode, NVMM_PATH and NVMM_SIZE are not needed; they are specified
   in the cluster config file.

   In addition, CACHE_SIZE is used to specify the kvs cache size in local library mode.

5. Run (multi-node, FAME)

   Load data from one node, noting the location of the KVS (LOC):
   ```
   $ ./ycsbc -db TYPE -threads CNT -P workloads/WORKLOAD -mode load
   ```

   Run benchmark from multiple nodes:
   ```
   $ ./ycsbc -db TYPE -threads CNT -P workloads/WORKLOAD -mode run -location LOC
   ```
