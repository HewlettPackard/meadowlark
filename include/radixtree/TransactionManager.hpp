#ifndef TRANSACTION_MANAGER_HPP
#define TRANSACTION_MANAGER_HPP

#include <assert.h>

#include "radixtree/radixtree_libpmem.h"
#include "radixtree/radixtree_fam_atomic.h"
#include "radixtree/RadixTree.hpp"
#include "radixtree/status.h"
#include "radixtree/log.h"

#include "nvmm/memory_manager.h"
#include "nvmm/heap.h"

namespace radixtree {

nvmm::PoolId const TXN_MGR_POOL_ID = 2;
    
class TransactionManager {
public:
    typedef uint64_t tid_t;

    static TransactionManager *getInstance();

    void acquireLock (tid_t tid);
    bool releaseLock (tid_t tid);
    tid_t peakLock ();

    tid_t getNextTid ();
    uint64_t getNextHandleId ();

    Status insertToCatalog (const std::string &indexName, const bool isUnique);
    Status dropFromCatalog (const std::string &indexName);

    Status lookupCatalog (nvmm::GlobalPtr &root, bool &isUnique, const std::string &indexName);

    inline nvmm::MemoryManager *getMmgr() {return mmgr;}
    inline nvmm::Heap *getHeap() {return heap;}

    // for test only!!!
    // delete all existing files and restart the transaction manager
    void Reset();

private:
    void* toLocal(const nvmm::GlobalPtr &gptr);
    uint64_t encodeMultiPtr(const nvmm::GlobalPtr &gptr);
    bool isMultiPtr(const uint64_t &gptr);
    nvmm::GlobalPtr decodeMultiPtr(const uint64_t &gptr);

private:

    static const size_t kRegionSize = 128*1024*1024; // 128MB
    static const size_t kHeapSize = 8*1024*1024*1024L; // 1024MB

    TransactionManager();
    ~TransactionManager();

    void Init(); // start the transaction manager using existing heap if available
    void Final(); // shutdown the transaction manager without deleting the heap

    nvmm::MemoryManager *mmgr;
    nvmm::Heap *heap;
    nvmm::Region *region;

    struct txn_mgr_header
    {
        tid_t glock;
        tid_t tidCounter;
        tid_t handleCounter;
        nvmm::GlobalPtr catalogRoot;
    };

    txn_mgr_header *header;
    nvmm::GlobalPtr catalogRoot;
    RadixTree *catalogIndex;
};

}

#endif
