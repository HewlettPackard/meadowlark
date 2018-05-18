# Consistent DRAM caching for FAM-aware Key-value store (KVS-DRAM-Cache)

Contact: Yupu Zhang (yupu.zhang@hpe.com)

## Description

KVS-DRAM-Cache is a caching layer that exposes the same KVS APIs as the underlying KVS
library and exploits node-local DRAM to provide fast and consistent caching. To provide consistency,
our caching approach uses tagged value pointers, value pointers with an embedded version number, to
check whether cached value data is up-to-date. When there is a cache hit, the value version in DRAM
cache is compared against the actual version in FAM. To avoid FAM index traversal, we make the
radixtree nodes long-lived (i.e., never deleted from the index once allocated) such that we can
cache a “short-cut” pointer to tree ndoes where the tagged value pointers are located.

We have implemented the caching layer based on Memcached. This variant of Memcached provides library
APIs for local access and server front-end for remote access.

KVS-DRAM-Cache relies on the FAM-aware Radix Tree library, which is available
from https://github.hpe.com/labs/radixtree or https://github.com/HewlettPackard/meadowlark.

## Master Source

https://github.com/HewlettPackard/meadowlark

## Maturity

KVS-DRAM-Cache is still in alpha state. The basic functionalities are working, but performance is not
optimized.


## Dependencies

- Install additional packages

  ```
  $ sudo apt-get install build-essential cmake libboost-all-dev
  ```

  You can specify custom path for libboost by setting an environment variable BOOST_ROOT
  ```
  $ export BOOST_ROOT=/opt/usr/local
  ```

- Install libevent packages

  ```
  libevent.org
  ```

- Install NVMM

  ```
  https://github.com/HewlettPackard/meadowlark
  ```

- Install FAM-aware Key-value store

  ```
  https://github.com/HewlettPackard/meadowlark
  ```


## Build & Test

We can change different configurations of cache policy with make command.

```
#########CACHE_MODE#########
#0: default memcached + KVS
#1: (full) store whole data block with short-cut pointer
#2: (short) only store short-cut pointer
#3: hybrid mode (hot-cold)
#4: KVS only, no cache
#5: cache only, no KVS

#########VERSION_MODE##########
#0: This number is used for partitioned mode that don't need version verification
#1: This number is used for version based sharing mode
#2: This number is used for version based sharing mode, and will force a read from FAM
```

For example, if you want to use hybrid cache with versioned mode,

```
make CACHE=3 VERSION=1
```

## Notes
- If you want to find the section changed from original memcached, use grep -r "CACHE_MODE" ./*

