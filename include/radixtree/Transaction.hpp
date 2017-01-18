#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <iostream>
#include <unordered_map>
#include <utility>
#include <assert.h>

#include "radixtree/radixtree_libpmem.h"
#include "radixtree/radixtree_fam_atomic.h"
#include "radixtree/RadixTree.hpp"
#include "radixtree/TransactionManager.hpp"
#include "radixtree/status.h"
#include "radixtree/log.h"

#include "nvmm/memory_manager.h"
#include "nvmm/heap.h"

namespace radixtree
{
class Transaction {
public:
    typedef RadixTree::Iter Iter;

    // Transaction ID
    typedef uint64_t tid_t;

    // Size of key or value if variable length
    static const int VARIABLE_LEN = -1;

    // Operation which does not require a txn ID
    //static const tid_t NO_TXN = -1;

    // Special character to indicate open boundary
    static const std::string OPEN_BOUNDARY;
    
    // Index handle    
    typedef struct 
    {
	bool valid;
	tid_t tid;
	uint64_t id;
	std::string indexName;
	Iter iter;
    } indexHandle;

    // Index access type    
    enum idxAccessType 
    {
        INDEX_READ_ONLY,
        INDEX_READ_WRITE    
    };


public:
    Transaction();

    Status startTxn (tid_t &tid /* returned */);

    Status commitTxn (bool &committed, /* returned */
			   const tid_t &tid);
                                     
    Status abortTxn (const tid_t &tid);
        
    Status createIndex (const tid_t &tid,
			     const std::string &indexName,
			     const bool unique = true,
			     const bool ordered = true, /* optional */
			     const bool ascending = true, /* optional */
			     const int keyLen = VARIABLE_LEN, /* -1 denotes variable length */
			     const int valueLen = VARIABLE_LEN /* -1 denotes variable length */);
                                 
    Status dropIndex (const tid_t &tid,
			   const std::string &indexName);

                                
    Status openIndex (indexHandle &ih, /* returned */
			   const tid_t &tid,
			   const std::string &indexName,
			   const idxAccessType &access /* RO vs. RW */);
                                
    Status closeIndex (indexHandle &ih,
			    const tid_t &tid);
                                
    Status insertIndexItem (indexHandle &ih,
				 const tid_t &tid,
				 const char *key,
				 const int keyLen,
				 const char *value,
				 const int valueLen);

    Status deleteIndexItem (indexHandle &ih,
				 const tid_t &tid,
				 const char *key, 
				 const int keyLen,
				 const char *val = NULL, 
				 const int valLen = 0);
                                      
    Status updateIndexItem (indexHandle &ih,
				 const tid_t &tid,
				 const char *key,
				 const int keyLen,
				 const char *value,
				 const int valueLen); /* assume this is only for value updates , not key updates */

    Status scanIndexItem (char *keyBuf, /* returned */
			       int &keyBufLen, /* returned */
			       char *valueBuf, /* returned */
			       int &valueBufLen, /* returned */
			       indexHandle &ih,
			       const tid_t &tid ,
			       const char *beginKey,
			       const int beginKeyLen,
			       const bool & beginInclusive,
			       const char *endKey,
			       const int endKeyLen,
			       const bool & endInclusive); /* can be used for point , range or full scans . Need to define MIN_VAL and MAX_VAL for open ranges . */

    Status nextIndexItem (char *keyBuf, /* returned */
			       int &keyBufLen, /* returned */
			       char *valueBuf, /* returned */
			       int &valueBufLen, /*returned */
			       indexHandle &ih ,
			       const tid_t &tid);
    
private:
    TransactionManager *tmgr;
};
    
}

#endif
