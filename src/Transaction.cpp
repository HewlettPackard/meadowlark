#include <radixtree/Transaction.hpp>

namespace radixtree
{

typedef RadixTree::Iter Iter;
typedef uint64_t tid_t;

const std::string Transaction::OPEN_BOUNDARY = "\x00";

Transaction::Transaction()
{
    tmgr = TransactionManager::getInstance();
}


Status Transaction::startTxn (tid_t &tid /* returned */)
{
    tid = tmgr->getNextTid();
    tmgr->acquireLock(tid);
    return Status::OK;
}

Status Transaction::commitTxn (bool & committed, /* returned */
		       const tid_t &tid)
{
    committed = true;
    tmgr->releaseLock(tid);
    return Status::OK;
}
                                     
Status Transaction::abortTxn (const tid_t &tid)
{
    tmgr->releaseLock(tid);
    return Status::OK;
}
        
Status Transaction::createIndex (const tid_t &tid,
				      const std::string &indexName,
				      const bool unique,
				      const bool ordered, /* optional */
				      const bool ascending, /* optional */
				      const int keyLen, /* -1 denotes variable length */
				      const int valueLen /* -1 denotes variable length */)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));
    
    return tmgr->insertToCatalog(indexName, unique);
}
                                 
Status Transaction::dropIndex (const tid_t &tid,
				    const std::string &indexName)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));
    
    return tmgr->dropFromCatalog(indexName);
}
                                
Status Transaction::openIndex (indexHandle &ih, /* returned */
				    const tid_t &tid,
				    const std::string &indexName,
				    const idxAccessType &access /* RO vs. RW */)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, indexName);
    if (!retCode.ok()) return retCode;
    
    ih.tid = tid;
    ih.id = tmgr->getNextHandleId();
    ih.indexName = indexName;
    ih.valid = true;
    
    return Status::OK;
}
   
Status Transaction::closeIndex (indexHandle &ih,
				     const tid_t &tid)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));
    
    ih.valid = false;
    
    return Status::OK;
}
    
Status Transaction::insertIndexItem (indexHandle &ih,
					  const tid_t &tid,
					  const char *key,
					  const int keyLen,
					  const char *value,
					  const int valueLen)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, ih.indexName);
    if (!retCode.ok()) return retCode;
    RadixTree index(root, tmgr->getMmgr(), tmgr->getHeap(), isUnique);
    
    //RadixTree index(tmgr->lookupCatalog(ih.indexName));
    
    return index.insert(key, keyLen, value, valueLen);
}
                                    
Status Transaction::deleteIndexItem (indexHandle &ih,
					  const tid_t &tid,
					  const char *key, 
					  const int keyLen,
					  const char *val, 
					  const int valLen)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, ih.indexName);
    if (!retCode.ok()) return retCode;
    RadixTree index(root, tmgr->getMmgr(), tmgr->getHeap(), isUnique);
    
    //RadixTree index(tmgr->lookupCatalog(ih.indexName));
    
    return index.remove(key, keyLen, val, valLen);
}
    
Status Transaction::updateIndexItem (indexHandle &ih,
					  const tid_t &tid,
					  const char *key,
					  const int keyLen,
					  const char *value,
					  const int valueLen) /* assume this is only for value updates , not key updates */
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, ih.indexName);
    if (!retCode.ok()) return retCode;
    RadixTree index(root, tmgr->getMmgr(), tmgr->getHeap(), isUnique);
    
    //RadixTree index(tmgr->lookupCatalog(ih.indexName));
    
    return index.update(key, keyLen, value, valueLen);
}

Status Transaction::scanIndexItem (char *keyBuf, /* returned */
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
					const bool & endInclusive) /* can be used for point , range or full scans . Need to define MIN_VAL and MAX_VAL for open ranges . */
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, ih.indexName);
    if (!retCode.ok()) return retCode;
    RadixTree index(root, tmgr->getMmgr(), tmgr->getHeap(), isUnique);
    
    //RadixTree index(tmgr->lookupCatalog(ih.indexName));
    
    return index.scan(keyBuf, keyBufLen, valueBuf, valueBufLen, ih.iter, beginKey, beginKeyLen, beginInclusive, endKey, endKeyLen, endInclusive);
}

Status Transaction::nextIndexItem (char *keyBuf, /* returned */
					int &keyBufLen, /* returned */
					char *valueBuf, /* returned */
					int &valueBufLen, /*returned */
					indexHandle &ih ,
					const tid_t &tid)
{
    if (tid != tmgr->peakLock())
	return Status(StatusCode::ABORTED, std::string("Invalid Transaction ID\n"));

    nvmm::GlobalPtr root;
    bool isUnique;
    Status retCode = tmgr->lookupCatalog(root, isUnique, ih.indexName);
    if (!retCode.ok()) return retCode;
    RadixTree index(root, tmgr->getMmgr(), tmgr->getHeap(), isUnique);
    
    //RadixTree index(tmgr->lookupCatalog(ih.indexName));
    
    return index.getNext(keyBuf, keyBufLen, valueBuf, valueBufLen, ih.iter);
}
    
}
    
