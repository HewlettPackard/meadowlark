#include <radixtree/TransactionManager.hpp>

#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED


namespace radixtree {

typedef uint64_t tid_t;

static inline uint64_t read64(uint64_t *addr)
{
    return fam_atomic_u64_read(addr);
}

static inline void write64(uint64_t *addr, uint64_t value) {
    return fam_atomic_u64_write(addr, value);
}
    
static inline uint64_t faa64(uint64_t *addr, uint64_t increment)
{
    return fam_atomic_u64_fetch_and_add(addr, increment);
}
    
static inline uint64_t cas64(uint64_t *addr, uint64_t oldval, uint64_t newval)
{
    return fam_atomic_u64_compare_and_store(addr, oldval, newval);
}
    /*
static inline void uint64ToStr (char *value, uint64_t addr) {
    value[0] = (char) (addr & 0xff);
    value[1] = (char) ((addr >> 8) & 0xff);
    value[2] = (char) ((addr >> 16) & 0xff);
    value[3] = (char) ((addr >> 24) & 0xff);
    value[4] = (char) ((addr >> 32)  & 0xff);
    value[5] = (char) ((addr >> 40) & 0xff);
    value[6] = (char) ((addr >> 48) & 0xff);
    value[7] = (char) ((addr >> 56) & 0xff);
}
    */
    
void* TransactionManager::toLocal(const nvmm::GlobalPtr &gptr) {
    return mmgr->GlobalToLocal(gptr);
}

uint64_t TransactionManager::encodeMultiPtr(const nvmm::GlobalPtr &gptr) {
    return (gptr.ToUINT64() | 0x0000000000000001);    
}
    
bool TransactionManager::isMultiPtr(const uint64_t &gptr) {
    return (gptr & 0x0000000000000001);
}

nvmm::GlobalPtr TransactionManager::decodeMultiPtr(const uint64_t &gptr) {
    return nvmm::GlobalPtr(gptr ^ 0x0000000000000001);
}

TransactionManager::TransactionManager ()
    : mmgr(NULL), heap(NULL), region(NULL), header(NULL)
{
    Init();
}

TransactionManager::~TransactionManager () {
    Final();
}

TransactionManager* TransactionManager::getInstance() {
    static TransactionManager tmgr;
    return &tmgr;
}

void TransactionManager::Init() {
    mmgr = nvmm::MemoryManager::GetInstance();
    nvmm::ErrorCode ec_mm;

    //create heap
    if (!(heap = mmgr->FindHeap(RADIXTREE_POOL_ID))) {
	if ((ec_mm = mmgr->CreateHeap(RADIXTREE_POOL_ID, kHeapSize)) != nvmm::NO_ERROR) { // initial heap size = 1GB
	    std::cout << "Heap alloc FAIL: " << ec_mm << "\n";
	    exit(1);
	}
	heap = mmgr->FindHeap(RADIXTREE_POOL_ID);
	assert(heap);
    }

    //open heap
    if ((ec_mm = heap->Open()) != nvmm::NO_ERROR) {
	std::cout << "Heap open FAIL: " << ec_mm << "\n";
	exit(1);
    }
    
    //create region if not exist
    if (!(region = mmgr->FindRegion(TXN_MGR_POOL_ID))) {
	if ((ec_mm = mmgr->CreateRegion(TXN_MGR_POOL_ID, kRegionSize)) != nvmm::NO_ERROR) { // initial region size = 1GB
	    std::cout << "Region alloc FAIL: " << ec_mm << "\n";
	    exit(1);
	}
	region = mmgr->FindRegion(TXN_MGR_POOL_ID);
	assert(region);
 
        //open region
        if ((ec_mm = region->Open(O_RDWR)) != nvmm::NO_ERROR) {
            std::cout << "Region open FAIL: " << ec_mm << "\n";
            exit(1);
        }

        //map region
        if ((ec_mm = region->Map(NULL, kRegionSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&header)) != nvmm::NO_ERROR) {
            std::cout << "Region map FAIL: " << ec_mm << "\n";
            exit(1);
        }
                
        // initialize glock, tidCounter, handleCounter, catalogRoot
        write64(&header->glock, 0);
        write64(&header->tidCounter, 1);
        write64(&header->handleCounter, 0);
        nvmm::GlobalPtr root = heap->Alloc(sizeof(RadixTree::Node));
        assert(root.IsValid() == true);
        write64((uint64_t*)&header->catalogRoot, root.ToUINT64());
        catalogRoot = root;
    }
    else
    {
        //open region
        if ((ec_mm = region->Open(O_RDWR)) != nvmm::NO_ERROR) {
            std::cout << "Region open FAIL: " << ec_mm << "\n";
            exit(1);
        }
        //map region
        if ((ec_mm = region->Map(NULL, kRegionSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&header)) != nvmm::NO_ERROR) {
            std::cout << "Region map FAIL: " << ec_mm << "\n";
            exit(1);
        }

        // load root from region
        nvmm::GlobalPtr root = nvmm::GlobalPtr(read64((uint64_t*)&header->catalogRoot));
        catalogRoot = root;
    }

    catalogIndex = new RadixTree(catalogRoot, mmgr, heap);
    assert(catalogIndex != NULL);        
}

void TransactionManager::Final() {
    if (catalogIndex) {
        delete catalogIndex;
    }
    
    if (region) {
        region->Unmap((void*)header, kRegionSize);
        region->Close();
        delete region;
    }
    if (heap) {
        heap->Close();
        delete heap;
    }

    mmgr = NULL;
    heap = NULL;
    region = NULL;
    header = NULL;
    catalogIndex = NULL;
}
    
void TransactionManager::Reset() {
    // remove everything and start from fresh
    Final();
    nvmm::ErrorCode ec_mm;
    nvmm::MemoryManager *mmgr_tmp = nvmm::MemoryManager::GetInstance();
    ec_mm = mmgr_tmp->DestroyHeap(RADIXTREE_POOL_ID);
    assert(ec_mm == nvmm::NO_ERROR);
    ec_mm = mmgr_tmp->DestroyRegion(TXN_MGR_POOL_ID);        
    assert(ec_mm == nvmm::NO_ERROR);
    Init();
}


    
void TransactionManager::acquireLock (tid_t tid) {
    while (cas64(&header->glock, 0, tid) != 0)
	;
}
    
bool TransactionManager::releaseLock (tid_t tid) {
    return (cas64(&header->glock, tid, 0) == tid);
}

tid_t TransactionManager::peakLock () {
    return read64(&header->glock);
    //return *glock;
}

tid_t TransactionManager::getNextTid () {
    //std::cout << "tidCounter_gptr = " << tidCounter_gptr << "\n";
    //std::cout << "tidCounter addr = " << tidCounter << "\n";
    //std::cout << "tidCounter = " << *tidCounter << "\n";

    return faa64(&header->tidCounter, 1);
}

uint64_t TransactionManager::getNextHandleId () {
    return faa64(&header->handleCounter, 1);
}

Status TransactionManager::insertToCatalog (const std::string &indexName, const bool isUnique) {
    
    nvmm::GlobalPtr newRoot = heap->Alloc(sizeof(RadixTree::Node));
    char value[8];
    int valueLen = 8;
    //uint64_t newRootAddr = newRoot.ToUINT64();
    uint64_t newRootAddr;
    if (isUnique)
	newRootAddr = newRoot.ToUINT64();
    else
	newRootAddr = encodeMultiPtr(newRoot);
    
    std::memcpy(value, &newRootAddr, 8);

    return catalogIndex->insert(indexName.c_str(), (int)indexName.length(), value, valueLen);
}
    
Status TransactionManager::dropFromCatalog (const std::string &indexName) {
    
    return catalogIndex->remove(indexName.c_str(), (int)indexName.length());
}

Status TransactionManager::lookupCatalog (nvmm::GlobalPtr &root, bool &isUnique, const std::string &indexName) {
    
    RadixTree::Iter iter;
    char keyBuf[4096]; //max index name length = 4096 bytes
    char valueBuf[8]; //64-bit pointers
    int keyBufLen = 4096;
    int valueBufLen = 8;
    Status retCode = catalogIndex->scan(keyBuf, keyBufLen, valueBuf, valueBufLen, iter, indexName.c_str(), (int)indexName.length(), true, indexName.c_str(), (int)indexName.length(), true);

    if (!retCode.ok())
	return Status(StatusCode::NOT_FOUND, std::string("lookupCatalog FAIL: index does not exist."));

    assert(valueBufLen == 8);

    uint64_t indexRootAddr = 0;
    std::memcpy(&indexRootAddr, valueBuf, valueBufLen);

    if (isMultiPtr(indexRootAddr)) {
	root = decodeMultiPtr(indexRootAddr);
	isUnique = false;
    }
    else {
	root = nvmm::GlobalPtr(indexRootAddr);
	isUnique = true;
    }

    return Status();
}

}
