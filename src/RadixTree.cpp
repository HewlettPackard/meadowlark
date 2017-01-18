#include <radixtree/RadixTree.hpp>
#include <radixtree/Transaction.hpp>

namespace radixtree {

typedef RadixTree::Node Node;
typedef RadixTree::Value Value;
typedef RadixTree::Iter Iter;

typedef nvmm::GlobalPtr Gptr;
typedef nvmm::MemoryManager Mmgr;
typedef nvmm::Heap Heap;

//*************************
// FAM Atomic Wrappers    *
//*************************
static inline uint32_t read32(uint32_t *addr) {
  return fam_atomic_u32_read(addr);
}

static inline uint64_t read64(uint64_t *addr) {
  return fam_atomic_u64_read(addr);
}

static inline void write32(uint32_t *addr, uint32_t value) {
  return fam_atomic_u32_write(addr, value);
}

static inline void write64(uint64_t *addr, uint64_t value) {
  return fam_atomic_u64_write(addr, value);
}

static inline uint32_t faa32(uint32_t *addr, uint32_t increment) {
  return fam_atomic_u32_fetch_and_add(addr, increment);
}

static inline uint64_t faa64(uint64_t *addr, uint64_t increment) {
  return fam_atomic_u64_fetch_and_add(addr, increment);
}

static inline uint32_t cas32(uint32_t *addr, uint32_t oldval, uint32_t newval) {
  return fam_atomic_u32_compare_and_store(addr, oldval, newval);
}

static inline uint64_t cas64(uint64_t *addr, uint64_t oldval, uint64_t newval) {
  return fam_atomic_u64_compare_and_store(addr, oldval, newval);
}

static inline Gptr readGptr(Gptr *gptr) {
  uint64_t gptr64 = read64((uint64_t*)gptr);
  return Gptr(gptr64);
}


//********************************
// Common Helpers                *
//********************************
void* RadixTree::toLocal(const Gptr &gptr) {
  return mmgr->GlobalToLocal(gptr);
}

inline bool RadixTree::isNullPtr(const Gptr &gptr) {
  return !(gptr.ToUINT64());
}

inline Gptr RadixTree::makeLeafPtr(const Gptr &gptr) {
  return Gptr(gptr.ToUINT64() | LEAF_BIT_MASK);
}

inline bool RadixTree::isLeafPtr(const Gptr &gptr) {
  return (gptr.ToUINT64() & LEAF_BIT_MASK);
}

Gptr RadixTree::getLeafValue(const Gptr &gptr) {
  return Gptr(gptr.ToUINT64() ^ LEAF_BIT_MASK);
}


//********************************
// Constructors                  *
//********************************
inline void RadixTree::init() {
  nvmm::PoolId pool_id = RADIXTREE_POOL_ID;
  mmgr = nvmm::MemoryManager::GetInstance();
  nvmm::ErrorCode ec_mm;

  //create heap
  if (!(heap = mmgr->FindHeap(pool_id))) {
    // initial heap size = 1GB
    if ((ec_mm = mmgr->CreateHeap(pool_id, 1024*1024*1024)) != nvmm::NO_ERROR) {
      std::cout << "Heap alloc FAIL: " << ec_mm << "\n";
      exit(1);
    }
    heap = mmgr->FindHeap(pool_id);
    assert(heap);
  }

  //open heap
  if ((ec_mm = heap->Open()) != nvmm::NO_ERROR) {
    std::cout << "Heap open FAIL: " << ec_mm << "\n";
    exit(1);
  }

  closeHeap = true;
}

RadixTree::RadixTree(bool u)
: isUnique(u), closeHeap(false)
{
  init();
  root = heap->Alloc(sizeof(Node));
}

RadixTree::RadixTree(Gptr r, bool u)
: root(r), isUnique(u), closeHeap(false)
{
  init();
}

RadixTree::RadixTree(Mmgr *m, Heap *h, bool u)
: mmgr(m), heap(h), isUnique(u), closeHeap(false)
{
  root = heap->Alloc(sizeof(Node));
}

RadixTree::RadixTree(Gptr r, Mmgr *m, Heap *h, bool u)
: mmgr(m), heap(h), root(r), isUnique(u), closeHeap(false)
{ }

RadixTree::~RadixTree() {
  if(closeHeap)
    heap->Close();
}


//===============================================================================================

// Create a value struct and flush it to persistent memory
//
// isKsuf indicates whether the value struct holds key suffix
// val_gptr is the returned global pointer of the created value struct
Value* RadixTree::createValue (const bool isKsuf, Gptr &val_gptr, const Gptr &next_gptr, const char *str, const unsigned len) {
  size_t valSize = sizeof(Value) + len;
  val_gptr = heap->Alloc(valSize);
  Value *val = (Value*) toLocal(val_gptr);
  val->isKeySuffix = (uint32_t)isKsuf;
  val->next = next_gptr;
  val->len = len;
  std::memcpy(val->data, str, len);

  pmem_persist(val, valSize);
  pmem_invalidate(val, valSize);
  return val;
}

// Create a empty node with specified prefix
//
// node_gptr is the returned global pointer of the created node
Node* RadixTree::createNode (Gptr &node_gptr, const char *prefix, const uint32_t prefixLen) {
  node_gptr = heap->Alloc(sizeof(Node));
  Node *node = (Node*) toLocal(node_gptr);
  node->prefixLen = prefixLen;
  std::memcpy(node->inlinePrefix, prefix, prefixLen);
  return node;
}

// Install the key suffix and the value to a given node
// This function is called during insertion when the tree traversal reaches the leaf node
// and the corresponding slot is empty
//
// isAtomic indicates whether the function should use atomic operations
// ksuf_gptr is the returned global pointer of the created key suffix
// val_gptr is the returned global pointer of the created leaf value
//******************************************************************************************
//
// Example: key = kimberly, value = keeton
//
//           --------------------------------
//           |         |k|                  |
//           --------------------------------
//                      |
//                      |
//                      V
//                  ---------
//                  |imberly|
//                  ---------
//                      |
//                      |
//                      V
//                   --------
//                   |keeton|
//                   --------
//
//******************************************************************************************
Status RadixTree::installKeyValue (const bool isAtomic, Gptr &ksuf_gptr, Gptr &val_gptr, Node *node, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen) {
  Gptr next_gptr = Gptr(0);
  createValue(false, val_gptr, next_gptr, value, valueLen); // create the leaf value

  // keyLen > 1 means that there is a key suffix and
  // it should be installed at position key[0] in the child array
  if (keyLen > 1) {
    uint8_t idx = (uint8_t) key[0]; // child array position
    createValue(true, ksuf_gptr, val_gptr, key + 1, keyLen - 1); // create the key suffix
    if (isAtomic) {
      if (cas64((uint64_t*)&(node->child[idx]), 0, makeLeafPtr(ksuf_gptr).ToUINT64()) != 0)
        return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
    } else
      node->child[idx] = makeLeafPtr(ksuf_gptr);
  }
  // keyLen == 1 means that there is NO key suffix and
  // the leaf value should be installed at position key[0] in the child array
  else if (keyLen == 1) {
    uint8_t idx = (uint8_t) key[0]; // child array position
    if (isAtomic) {
      if (cas64((uint64_t*)&(node->child[idx]), 0, makeLeafPtr(val_gptr).ToUINT64()) != 0)
        return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
    }
    else
      node->child[idx] = makeLeafPtr(val_gptr);
  }
  // keyLen == 0 means that there is NO key suffix and
  // the leaf value should be installed at node->val (position FANOUT)
  else {
    if (isAtomic) {
      if (cas64((uint64_t*)&(node->val), 0, val_gptr.ToUINT64()) != 0)
        return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
    }
    else
      node->val = val_gptr;
  }

  return Status(); //OK
}

//********************************************************************
// Example:
//    
// Before:  _                 After:       _
//         |P|                            |P|
//         |R|                            |R|
//         |E|                            |E|
//         |F|<--breakPos           -----------------
//         |I|                      | N E W N O D E |
//         |X|                      -F-------------X- <-- idx
//   ---------------                 |             |
//   |   N O D E   |                 |             |
//   ---------------                 V             V
//      |      |                     _       --------------
//      |      |                    |I|      | key suffix |
//      V      V                    |X|      --------------
//                            ---------------      |
//                            |   N O D E   |      |
//                            ---------------      V
//                               |      |      ---------
//                               |      |      | value |
//                               V      V      ---------
//
//********************************************************************
Status RadixTree::splitPrefix (unsigned breakPos, Gptr &parent_gptr, int parent_pos, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen) {
  Node *parent = (Node*) toLocal(parent_gptr);
  Gptr node_gptr = readGptr(&(parent->child[parent_pos]));

  Node *node = (Node*) toLocal(node_gptr);
  Gptr newNode_gptr, newChild_gptr, ksuf_gptr, val_gptr;

  uint64_t prefix64 = read64((uint64_t*)node->inlinePrefix);
  uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);

  char *newNodePrefix = (char*) &prefix64; // "PRE"
  int newNodePrefixLen = breakPos;
  char *nodePrefix = newNodePrefix + breakPos + 1; //"IX"
  int nodePrefixLen = prefixLen - breakPos - 1;
  uint8_t nodeIdx = (uint8_t)newNodePrefix[breakPos]; // "F"

  //create NEWNODE with prefix "PRE"
  Node *newNode = createNode(newNode_gptr, newNodePrefix, newNodePrefixLen);

  //create/copy NODE with new prefix "IX"
  Node *newChild = createNode(newChild_gptr, nodePrefix, nodePrefixLen);
  newChild->val = node->val;
  std::memcpy(newChild->child, node->child, sizeof(Gptr) * RadixTree::FANOUT);
  pmem_persist(newChild, sizeof(Node));
  pmem_invalidate(newChild, sizeof(Node));

  newNode->child[nodeIdx] = newChild_gptr; //link NODE to NEWNODE

  //link key suffix and value to NEWNODE
  installKeyValue(false, ksuf_gptr, val_gptr, newNode, key, keyLen, value, valueLen);

  pmem_persist(newNode, sizeof(Node));

  // install NEWNODE to parent
  if (cas64((uint64_t*)&(parent->child[parent_pos]), node_gptr.ToUINT64(), newNode_gptr.ToUINT64()) != node_gptr.ToUINT64())
    return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));

  pmem_invalidate(newNode, sizeof(Node));

  heap->Free(node_gptr);
  return Status(); //OK
}


//***************************************************************************
// Example: NODE already has (kimberly, cool), add (kimchi, yummy) to NODE
//
// Before:                 After:
//
//      -----------        -----------
//      | N O D E |        | N O D E |
//      ---k-------        ---k-------
//         |                  |
//         |                  |
//         V                  V
//        ---                ---
//        |i|                |i|
//        |m|                |m|
//        |b|         -----------------
//        |e|         | N E W N O D E |
//        |r|         -----b-------c---
//        |l|              |       |
//        |y|              |       |
//        ---              V       V
//         |              ---     ---
//         |              |e|     |h|
//         V              |r|     |i|
//       ------           |l|     ---
//       |cool|           |y|      |
//       ------           ---      |
//                         |       V
//                         |    -------
//                         V    |yummy|
//                       ------ -------
//                       |cool|
//                       ------
//
//***************************************************************************
Status RadixTree::splitSuffix (unsigned breakPos, Gptr &parent_gptr, int parent_pos, Gptr &old_child_gptr, Value *ksuf, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen) {
  Gptr newNode_gptr;
  Gptr firstNode_gptr, prevNode_gptr;
  Node *newNode = NULL, *prevNode = NULL;
  // find the position where the remaining suffix and the suffix stored in the node fail to match

  // create the first new node
  int32_t rSufPos = 0, prevPos = -1;
  do
  {
    if(prevPos != -1)
      rSufPos++;

    unsigned cnt = (unsigned)(breakPos - rSufPos);
    cnt = cnt>MAX_INLINE_PREFIX_LEN? MAX_INLINE_PREFIX_LEN : cnt;
    newNode = createNode(newNode_gptr, key+rSufPos, cnt);
    pmem_persist(newNode, sizeof(newNode)); // persist NEWNODE

    rSufPos+=cnt;

    if (firstNode_gptr.IsValid() == false)
    {
      firstNode_gptr = newNode_gptr;
    }
    else
    {
      // we have a prev node
      if (prevPos != -1)
      {
        // link NEWNODE to NODE
        if (cas64((uint64_t*)&(prevNode->child[prevPos]), 0, newNode_gptr.ToUINT64()) != 0)
          return Status(StatusCode::ABORTED, std::string("Insert FAIL 2: other concurrent transactions have modified the read set."));
      }
    }

    prevNode_gptr = newNode_gptr;
    prevNode = newNode;
    prevPos = ((unsigned)(rSufPos+1)<=breakPos)? (int32_t)key[rSufPos] : -1;
  }
  while((unsigned)rSufPos < breakPos);

  //// create NEWNODE with suffix "im"
  //Node *newNode = createNode(newNode_gptr, key, breakPos);

  Gptr oldVal_gptr = ksuf->next; // "cool"
  Gptr newVal_gptr;
  Gptr next_gptr = Gptr(0);

  createValue(false, newVal_gptr, next_gptr, value, valueLen); // create "yummy"

  // install old key suffix ("berly") and old value ("cool") to NEWNODE
  if (breakPos + 1 >= ksuf->len) { // if there is no key suffix needed
    if (breakPos >= ksuf->len) // if the key is just "kim"
      newNode->val = oldVal_gptr;
    else {
      uint8_t idx_old = ksuf->data[breakPos];
      newNode->child[idx_old] = makeLeafPtr(oldVal_gptr);
    }
  }
  else {
    Gptr oldKsuf_gptr;
    createValue(true, oldKsuf_gptr, oldVal_gptr, ksuf->data + breakPos + 1, ksuf->len - breakPos - 1); // create old key suffix "erly" followed by old value "cool"
    uint8_t idx_old = ksuf->data[breakPos]; // "b"
    newNode->child[idx_old] = makeLeafPtr(oldKsuf_gptr); // install "erly"
  }

  // install new key suffix ("chi") and new value ("yummy") to NEWNODE
  if (breakPos + 1 >= keyLen) { // if there is no key suffix needed
    if (breakPos >= keyLen) // if the key is just "kim"
      newNode->val = newVal_gptr;
    else {
      uint8_t idx_new = key[breakPos];
      newNode->child[idx_new] = makeLeafPtr(newVal_gptr);
    }
  }
  else {
    Gptr newKsuf_gptr;
    createValue(true, newKsuf_gptr, newVal_gptr, key + breakPos + 1, keyLen - breakPos - 1); // create new key suffix "hi" followed by new value "yummy"
    uint8_t idx_new = key[breakPos]; // "c"
    newNode->child[idx_new] = makeLeafPtr(newKsuf_gptr); // install "hi"
  }

  pmem_persist(newNode, sizeof(newNode)); // persist NEWNODE

  // // link NEWNODE to NODE
  // Node *parent = (Node*) toLocal(parent_gptr);
  // if (cas64((uint64_t*)&(parent->child[parent_pos]), old_child_gptr.ToUINT64(), newNode_gptr.ToUINT64()) != old_child_gptr.ToUINT64())
  //     return Status(StatusCode::ABORTED, std::string("Insert FAIL 2: other concurrent transactions have modified the read set."));

  // pmem_invalidate(newNode, sizeof(newNode));

  // link first node to parent node
  Node *parent = (Node*) toLocal(parent_gptr);
  if (cas64((uint64_t*)&(parent->child[parent_pos]), old_child_gptr.ToUINT64(), firstNode_gptr.ToUINT64()) != old_child_gptr.ToUINT64())
    return Status(StatusCode::ABORTED, std::string("Insert FAIL 2: other concurrent transactions have modified the read set."));

  pmem_invalidate(newNode, sizeof(newNode));

  return Status(); //OK
}


//***************************************************************************
// For non-unique index only:
// Append the new value to the end of the value chain
//***************************************************************************
Status RadixTree::appendValue (Gptr &val_gptr, const char *value, const unsigned valueLen) {
  Gptr newVal_gptr;
  Value* currVal = (Value*) toLocal(val_gptr);
  Gptr currValNext_gptr = readGptr(&currVal->next);

  if (isNullPtr(currValNext_gptr)) {
    Gptr next_gptr = Gptr(0);
    createValue(false, newVal_gptr, next_gptr, value, valueLen);
    if (cas64((uint64_t*)&(currVal->next), 0, newVal_gptr.ToUINT64()) != 0)
        return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
  } else {
    // Let's try to add the new value at this position in the list
    createValue(false, newVal_gptr, currValNext_gptr, value, valueLen);
    uint64_t next_tmp = currValNext_gptr.ToUINT64();
    if (cas64((uint64_t*)&(currVal->next), next_tmp, newVal_gptr.ToUINT64()) != next_tmp)
      return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));

  }


  // Walk to the end of the list and append the value. This is very slow for a long lists...
//  Gptr next_gptr = Gptr(0);
//  createValue(false, newVal_gptr, next_gptr, value, valueLen);
//  while (!isNullPtr(currValNext_gptr)) {
//    currVal = (Value*) toLocal(currValNext_gptr);
//    currValNext_gptr = readGptr(&currVal->next);
//  }
//  if (cas64((uint64_t*)&(currVal->next), 0, newVal_gptr.ToUINT64()) != 0)
//    return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));

  return Status();
}

//***************************************************************************
// Recursive call for insert
//***************************************************************************
Status RadixTree::insertToNode (Gptr &parent_gptr, int parent_pos, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen) {
    Gptr node_gptr;
    if (isNullPtr(parent_gptr))
        node_gptr = root;
    else {
        Node *parent=(Node*) toLocal(parent_gptr);
        node_gptr = readGptr(&(parent->child[parent_pos]));
    }
  Node *node = (Node*) toLocal(node_gptr);
  unsigned keyPos = 0;

  //*********************
  // Handle Prefix
  //*********************
  //*********************************************************************************
  //                                    |
  // CASE 1:                            |    CASE 2:
  //                                    |
  // node:      key:                    |    node:       key:
  //     ---       ---                  |        ---        ---
  //     |k|       |k|                  |        |k|        |k|
  //     |i|       |i|                  |        |i|        |i|
  //     |m|       |m|                  |        |m|        |m|
  //     |b|       |c| <--keyPos        |        |b|        --- <--keyPos == keyLen
  //     |e|       |h|                  |        |e|
  //     |r|       |i|                  |        |r|
  //     |l|       --- <--keyLen        |        |l|
  //     |y|                            |        |y|
  // ----------- <--prefixLen           |    ----------- <--prefixLen
  // | N O D E |                        |    | N O D E |
  // -----------                        |    -----------
  //                                    |
  //--------------------------------------------------------------------------------
  //                                    |
  // CASE 3:                            |    CASE 4:
  //                                    |
  // node:      key:                    |    node:       key:
  //     ---       ---                  |        ---        ---
  //     |k|       |k|                  |        |k|        |k|
  //     |i|       |i|                  |        |i|        |i|
  //     |m|       |m|                  |        |m|        |m|
  //     |b|       |b|                  |        |b|        |b|
  //     |e|       |e|                  |        |e|        |e|
  //     |r|       |r|                  |        |r|        |r|
  //     |l|       |l|                  |        |l|        |l|
  //     |y|       |y|                  |        |y|        |y|
  // -----------   --- <--keyPos        |    -----------    |k| <-- keyPos == prefixLen
  // | N O D E |          == keyLen     |    | N O D E |    |e|
  // -----------          == prefixLen  |    -----------    |e|
  //                                    |                   |t|
  //                                    |                   |o|
  //                                    |                   |n|
  //                                    |                   --- <--keyLen
  //                                    |
  //*********************************************************************************
  uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);
  uint64_t prefix64 = read64((uint64_t*)node->inlinePrefix);
  char *prefix = (char*)&prefix64;

  // std::cout << "!! " << keyLen << " " << valueLen << "?? " << prefixLen << " " << std::string(prefix, 8) << std::endl;
  // std::string tmp_key(key, keyLen);
  // std::string tmp_val(value, valueLen);
  // std::cout << "!! " << tmp_key << " " << tmp_val << std::endl;
  //printRoot();

  if (prefixLen > 0) { // if the node has prefix
    // find the position where the key and the prefix fail to match
    while (keyPos < keyLen && keyPos < prefixLen && key[keyPos] == prefix[keyPos])
      keyPos++;

    // if the key and the prefix fail to match at keyPos, the prefix needs a split
    if (keyPos < prefixLen) {
      if (keyPos < keyLen) // CASE 1: if there is remaining key after keyPos
        return splitPrefix(keyPos, parent_gptr, parent_pos, key + keyPos, keyLen - keyPos, value, valueLen);
      else // CASE 2
        return splitPrefix(keyPos, parent_gptr, parent_pos, NULL, 0, value, valueLen);
    }
  }

  //**********************************************************
  // CASE 3, 4
  // The node has no prefix or the key and the prefix match
  // Continue traversing the radix tree
  //**********************************************************
  const char *rKey = key + keyPos; //remaining key, "keeton" in CASE 4
  unsigned rKeyLen = keyLen - keyPos;
  const char *rSuf = key + keyPos + 1; //remaining suffix, "eeton" in CASE 4
  unsigned rSufLen = keyLen - keyPos - 1;

  // CASE 3: if the key and the prefix match but there is no remaining key after keyPos
  Gptr ksuf_gptr, val_gptr;
  if (rKeyLen == 0) {
    Gptr nodeVal_gptr = readGptr(&node->val);
    if (isNullPtr(nodeVal_gptr))
      return installKeyValue(true, ksuf_gptr, val_gptr, node, rKey, rKeyLen, value, valueLen);
    else {
      if (isUnique)
        return Status(StatusCode::ALREADY_EXISTS, std::string("Insert FAIL: key already exists."));
      else
        return appendValue(nodeVal_gptr, value, valueLen);
    }
  }

  //CASE 4 ---------------------------------------------
  Gptr child_gptr = readGptr(&node->child[(uint8_t)key[keyPos]]); // key[keyPos] is the index to look in the child array, or "k" in CASE 4
  if (isNullPtr(child_gptr)) { // if the position ("k") in the child array is empty
    return installKeyValue(true, ksuf_gptr, val_gptr, node, rKey, rKeyLen, value, valueLen);
  }

  if (isLeafPtr(child_gptr)) { // if the position ("k") in the child array already has a key sufix or value
    //std::cout << "!!! case 4 leafptr " << std::endl;
    Gptr fetched_child_gptr = getLeafValue(child_gptr);
    Value *val = (Value*) toLocal(fetched_child_gptr);
    if (val->isKeySuffix) { // if the position ("k") in the child array points to a key suffix
      Value *ksuf = val;
      uint32_t ksufLen = ksuf->len;

      unsigned rSufPos = 0;
      // find the position where the remaining suffix and the suffix stored in the node fail to match
      while (rSufPos < rSufLen && rSufPos < ksufLen && rSuf[rSufPos] == ksuf->data[rSufPos])
        rSufPos++;

      // if the remaining suffix and the suffix stored in the node are exactly the same
      if (rSufPos >= rSufLen && rSufPos >= ksufLen) {
        if (isUnique) {
          pmem_invalidate(ksuf, sizeof(ksuf) + ksufLen);
          return Status(StatusCode::ALREADY_EXISTS, std::string("Insert FAIL: key already exists."));
        }
        else
          return appendValue(ksuf->next, value, valueLen);
      }
      else // otherwise, the stored suffix needs a split
      {
        //std::cout<< "!! split suffix " << rSufPos << " " << rSufLen << std::endl;
        return splitSuffix(rSufPos, node_gptr, (int)key[keyPos], child_gptr, ksuf, rSuf, rSufLen, value, valueLen);
      }
    }
    else { // if the position ("k") in the child array points to a value
      if (rSufLen == 0) {
        if (isUnique)
          return Status(StatusCode::ALREADY_EXISTS, std::string("Insert FAIL: key already exists."));
        else
          return appendValue(fetched_child_gptr, value, valueLen);
      }
      Gptr newNode_gptr;
      Node *newNode = createNode(newNode_gptr, NULL, 0);
      installKeyValue(false, ksuf_gptr, val_gptr, newNode, rSuf, rSufLen, value, valueLen);
      newNode->val = getLeafValue(child_gptr);

      pmem_persist(newNode, sizeof(newNode));

      if (cas64((uint64_t*)&(node->child[(uint8_t)rKey[0]]), child_gptr.ToUINT64(), newNode_gptr.ToUINT64()) != child_gptr.ToUINT64())
        return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));

      pmem_invalidate(newNode, sizeof(newNode));
    }
  }
  else // if the position ("k") in the child array points to a child node
  {
    //std::cout << "!!! case 4 else " << std::endl;
    return insertToNode(node_gptr, (int)key[keyPos], rSuf, rSufLen, value, valueLen);
  }

  return Status(); //OK
}


//************
// INSERT    *
//************
Status RadixTree::insert (const char *key, const int keyLen, const char *value, const int valueLen) {
  Gptr empty = Gptr(0);
  return insertToNode (empty, 0, key, (unsigned)keyLen, value, (unsigned)valueLen);
}

//===============================================================================================

//***************************************************************************
// Recursive call for point query
//***************************************************************************
bool RadixTree::findRecursive (Iter &iter) {
  const char *key = iter.key.c_str() + iter.keyPos;
  unsigned keyLen = (unsigned)iter.key.length() - iter.keyPos;
  Gptr node_gptr = (iter.pos >= FANOUT) ? iter.node : readGptr(&((Node*) toLocal(iter.node))->child[iter.pos]);

  if (isNullPtr(node_gptr)) return false;

  Node* node = (Node*) toLocal(node_gptr);
  unsigned keyPos = 0;

  // handle prefix ------------------------------------------------------------
  uint64_t prefix64 = read64((uint64_t*)node->inlinePrefix);
  uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);
  char *prefix = (char*) &prefix64;

  if (prefixLen > 0) { // if the node has prefix
    // find the position where the key and the prefix fail to match
    while (keyPos < keyLen && keyPos < prefixLen && key[keyPos] == prefix[keyPos])
      keyPos++;
    // prefix doesn't match, key does NOT found
    if (keyPos < prefixLen)
      return false;
  }

  iter.node = node_gptr;
  iter.keyPos += keyPos;

  // if the key and the prefix match but there is NO remaining key after keyPos
  Gptr child_gptr;
  if (keyPos >= keyLen) {
    child_gptr = readGptr(&node->val);
    if (!isNullPtr(child_gptr)) {
      iter.pos = FANOUT;
      return true;
    }
    return false;
  }

  // if the key and the prefix match but there is remaining key after keyPos
  uint8_t idx = (uint8_t)key[keyPos];
  child_gptr = readGptr(&(node->child[idx]));

  if (isLeafPtr(child_gptr)) {
    const char *rSuf = key + keyPos + 1; //remaining suffix
    unsigned rSufLen = keyLen - keyPos - 1;
    Gptr fetched_child_gptr = getLeafValue(child_gptr);
    Value *val = (Value*) toLocal(fetched_child_gptr);
    if (val->isKeySuffix) { // if the reached leaf slot has suffix
      Value *ksuf = val;
      uint32_t ksufLen = ksuf->len;
      if (rSufLen != ksufLen) { //key suffix length mismatch
        pmem_invalidate(ksuf, sizeof(ksuf) + ksufLen);
        return false;
      }

      for (unsigned i = 0; i < ksufLen; i++) {
        if (rSuf[i] != ksuf->data[i]) { //key suffix mismatch
          pmem_invalidate(ksuf, sizeof(ksuf) + ksufLen);
          return false;
        }
      }
      pmem_invalidate(ksuf, sizeof(ksuf) + ksufLen);
    }
    else {
      if (rSufLen != 0) // the leaf slot has no suffix, but there is remaining key
        return false;
    }

    iter.pos = idx;
    return true;
  }

  iter.nodePath.push_back(iter.node); // record node traverse history
  iter.node = child_gptr;
  iter.keyPos += 1;

  return findRecursive(iter);
}

//***************************************************************************
// Point query
//***************************************************************************
bool RadixTree::find (Iter &iter, const char *key, const unsigned keyLen) {
  iter.node = root;
  iter.pos = FANOUT;
  iter.key = std::string(key, keyLen);
  iter.keyPos = 0;
  return findRecursive(iter);
}


//***************************************************************************
// Find the left most leaf of the current subtree indicated by the iterator
//
// Example:
//                    ---------
//                    |N O D E| <--iter.node.child[iter.pos]
//                    ---------
//                    |       |
//                    |       |
//                    V       V
//               --------- ---------
//               |N O D E| |N O D E|
//               --------- ---------
//               |       |         |
//               |       |         |
//               V       V         V
//          --------- --------- ---------
//          |N O D E| |N O D E| |N O D E|
//          --------- --------- ---------
//             | <------ the function will find here
//             |
//             V
//        -----------
//        |KEYSUFFIX|
//        -----------
//             |
//             |
//             V
//          -------
//          |VALUE|
//          -------
//
//***************************************************************************
bool RadixTree::leftMost (Iter &iter) {
  if (isNullPtr(iter.node)) return false;

  Node* parent = (Node*) toLocal(iter.node);
  Gptr val_gptr;
  LOG(debug) << "leftMost on node " <<
        iter.node << " on pos " << iter.pos << "(" << (char)iter.pos << ")" <<
      ": current key(" << iter.key.length() << ") = " << iter.key
      << " with keyPos key[" << iter.keyPos << "]="
      << (iter.keyPos < iter.key.length() ? iter.key[iter.keyPos] : '-');
  
  if (iter.pos == FANOUT) { // position is node->val
    val_gptr = readGptr(&parent->val);
    Value* val = (Value*) toLocal(val_gptr);
    if (val->isKeySuffix)
      iter.key += std::string(val->data, val->len);

    pmem_invalidate(val, sizeof(val) + val->len);
    return true;
  }

  Gptr node_gptr = readGptr(&parent->child[iter.pos]);
  if (isLeafPtr(node_gptr)) { // if reached leaf
    val_gptr = getLeafValue(node_gptr);
    Value* val = (Value*) toLocal(val_gptr);
    if (val->isKeySuffix)
      iter.key += std::string(val->data, val->len);

    pmem_invalidate(val, sizeof(val) + val->len);
    return true;
  }

  // haven't reached leaf yet-----------------------------------------
  Node* node = (Node*) toLocal(node_gptr);

  iter.nodePath.push_back(iter.node); // record node traverse history
  iter.node = node_gptr;

  uint64_t prefix64 = read64((uint64_t*)node->inlinePrefix);
  uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);
  char *prefix = (char*) &prefix64;
  if (prefixLen > 0) { // handle prefix
    iter.keyPos += prefixLen;
    iter.key = iter.key.substr(0, iter.keyPos);
    iter.key += std::string(prefix, prefixLen);
  }

  // check node->val first because it is at the left most
  Gptr nodeVal_gptr = readGptr(&node->val);
  if (!isNullPtr(nodeVal_gptr)) {
    iter.pos = FANOUT;
    // BUG
    iter.keyPos++;
    return true;
  }

  // then check the child array in order
  unsigned idx = 0;
  Gptr child_gptr = readGptr(&node->child[idx]);
  while (isNullPtr(child_gptr)) {
    idx++;
    child_gptr = readGptr(&node->child[idx]);
  }

  iter.pos = idx;
  iter.keyPos++;
  iter.key = iter.key.substr(0, iter.keyPos);
  iter.key.push_back((char)idx);

  return leftMost(iter);
}


//***************************************************************************
// Recursive call for partial key search
//***************************************************************************
bool RadixTree::lowerBoundRecursive (Iter &iter, bool isInclusive) {
  const char *key = iter.key.c_str() + iter.keyPos;
  unsigned keyLen = (unsigned)iter.key.length() - iter.keyPos;
  Gptr node_gptr = (iter.pos >= FANOUT) ? iter.node : readGptr(&((Node*) toLocal(iter.node))->child[iter.pos]);
  if (isNullPtr(node_gptr)) return false;

  LOG(debug) << "lowerBoundRecursive on node " <<
        iter.node << " on pos " << iter.pos << "(" << (char)iter.pos << ")" <<
      ": current key(" << iter.key.length() << ") = " << iter.key
      << " with keyPos key[" << iter.keyPos << "]="
      << (iter.keyPos < iter.key.length() ? iter.key[iter.keyPos] : '-');


  Node* node = (Node*) toLocal(node_gptr);
  unsigned keyPos = 0;

  // handle prefix ------------------------------------------------------------
  uint64_t prefix64 = read64((uint64_t*)node->inlinePrefix);
  uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);
  char *prefix = (char*) &prefix64;
  if (prefixLen > 0) {
    // find the position where the key and the prefix fail to match
    while (keyPos < keyLen && keyPos < prefixLen && key[keyPos] == prefix[keyPos]) {
      keyPos++;
      iter.keyPos++;
    }
    // prefix doesn't match
    if (keyPos < prefixLen) {
      if (keyPos >= keyLen || key[keyPos] < prefix[keyPos])
      {
        LOG(debug) << "!! lowerBoundRecursive 1 leftMost " << std::string(key, keyPos) << " keyPos " << keyPos ;
        return leftMost(iter);
      }
      else
      {
        LOG(debug) << "!! lowerBoundRecursive 1 nextLeaf " << std::string(key, keyPos) << " keyPos " << keyPos ;
        return nextLeaf(iter);
      }
    }
  }

  iter.node = node_gptr;

  // if the key and the prefix match but there is NO remaining key after keyPos
  Gptr child_gptr;
  if (keyPos >= keyLen) {
    iter.pos = FANOUT;
    child_gptr = readGptr(&node->val);
    // if (!isNullPtr(child_gptr)) {
    //   if (!isInclusive)
    //   {
    //       LOG(debug) << "!! lowerBoundRecursive 2 nextLeaf of node (" << iter.node << ") " << std::string(key, keyPos) << " keyPos " << keyPos ;
    //     return nextLeaf(iter);
    //   }
    //   return true;
    // }
    // else
    // {
    //     iter.pos = FANOUT+1;
    // }
    if (!isNullPtr(child_gptr) && isInclusive)
    {
        LOG(debug) << "!! lowerBoundRecursive 2 out (" << iter.node << ") " << std::string(key, keyPos) << " keyPos " << keyPos ;
        return true;
    }

    //iter.pos = FANOUT+1;
    LOG(debug) << "!! lowerBoundRecursive 3 nextLeaf " << std::string(key, keyPos) << " keyPos " << keyPos << " fanout " << iter.pos ;
    return nextLeaf(iter);
  }

  // if the key and the prefix match but there is remaining key after keyPos
  uint8_t idx = (uint8_t)key[keyPos];
  child_gptr = readGptr(&(node->child[idx]));

  if (isNullPtr(child_gptr)) {
    iter.pos = idx;
    LOG(debug) << "!! lowerBoundRecursive 4 nextLeaf " << std::string(key, keyPos) << " keyPos " << keyPos ;
    return nextLeaf(iter);
  }

  if (isLeafPtr(child_gptr)) {
    iter.pos = idx;
    if (!isInclusive)
    {
      LOG(debug) << "!! lowerBoundRecursive 5 nextLeaf " << std::string(key, keyPos) << " keyPos " << keyPos ;
      return nextLeaf(iter);
    }
    LOG(debug) << "!! lowerBoundRecursive 6 out " << std::string(key, keyPos) << " keyPos " << keyPos ;
    return true;
  }

  iter.nodePath.push_back(iter.node); // record node traverse history
  iter.node = node->child[idx];
  iter.keyPos += 1;

  LOG(debug) << "!! lowerBoundRecursive 7 out " << std::string(key, keyPos) << " keyPos " << keyPos ;  
  return lowerBoundRecursive(iter, isInclusive);
}

//***************************************************************************
// Range Query: partial key search
//***************************************************************************
bool RadixTree::lowerBound (Iter &iter, const char *key, const unsigned keyLen, bool isInclusive) {
  iter.node = root;
  iter.pos = FANOUT + 1;
  iter.key = std::string(key, keyLen);
  iter.keyPos = 0;
  return lowerBoundRecursive(iter, isInclusive);
}


//************
// SCAN      *
//************
//***************************************************************************
// keyBuf, keyBufLen, valueBuf, valueBufLen are returned key value
//***************************************************************************
Status RadixTree::scan (char *keyBuf, int &keyBufLen, char *valueBuf, int &valueBufLen, Iter &iter, const char *beginKey, const int beginKeyLen, const bool beginKeyInclusive, const char *endKey, const int endKeyLen, const bool endKeyInclusive) {
  iter.beginKey = std::string(beginKey, beginKeyLen);
  iter.beginKeyInclusive = beginKeyInclusive;
  if (iter.beginKey == Transaction::OPEN_BOUNDARY) { // handle begin open boundary
    iter.beginKey = std::string("\0", 1);
    iter.beginKeyInclusive = true;
  }

  iter.endKey = std::string(endKey, endKeyLen);
  iter.endKeyInclusive = endKeyInclusive;

  // if the scan is a point query --------------------------------------------------
  if (iter.beginKey == iter.endKey && beginKeyInclusive && endKeyInclusive) {
    if (find(iter, beginKey, beginKeyLen)) {
      //key buffer size check
      if (keyBufLen < beginKeyLen)
        return Status(StatusCode::OUT_OF_RANGE, std::string("scan FAIL: key buffer NOT large enough."));
      keyBufLen = beginKeyLen;
      memcpy(keyBuf, beginKey, keyBufLen);

      Value* val = getValue(iter);
      //value buffer size check
      if (valueBufLen < (int)val->len)
        return Status(StatusCode::OUT_OF_RANGE, std::string("scan FAIL: value buffer NOT large enough."));
      valueBufLen = val->len;
      memcpy(valueBuf, val->data, valueBufLen);

      return Status(); //OK
    }
    return Status(StatusCode::NOT_FOUND, std::string("scan FAIL: no item in the range."));
  }

  // if the scan is a range query --------------------------------------------------

  //printRoot();

  if ((iter.endKey == Transaction::OPEN_BOUNDARY) || (iter.beginKey < iter.endKey)) {
    if (lowerBound(iter, iter.beginKey.c_str(), (unsigned)iter.beginKey.length(), iter.beginKeyInclusive)) {
      if ((iter.endKey != Transaction::OPEN_BOUNDARY) && ((iter.key > iter.endKey) || ((iter.key == iter.endKey) && !endKeyInclusive)))
        return Status(StatusCode::OUT_OF_RANGE, std::string("scan FAIL: no item in the range."));

      //key buffer size check
      if (keyBufLen < (int)iter.key.length())
        return Status(StatusCode::OUT_OF_RANGE, std::string("scan FAIL: key buffer NOT large enough."));
      keyBufLen = (int)iter.key.length();
      memcpy(keyBuf, iter.key.c_str(), keyBufLen);

      Value* val = getValue(iter);
      //value buffer size check
      if (valueBufLen < (int)val->len)
        return Status(StatusCode::OUT_OF_RANGE, std::string("scan FAIL: value buffer NOT large enough."));
      valueBufLen = val->len;
      memcpy(valueBuf, val->data, valueBufLen);

      return Status(); //OK
    }
    return Status(StatusCode::NOT_FOUND, std::string("scan FAIL: no item in the range."));
  }

  return Status(StatusCode::NOT_FOUND, std::string("scan FAIL: invalid range."));
}


//***************************************************************************
// Find the next NON-empty node slot given the current iterator
//    
// Example:
//                    -----------------------------
//                    | N       O       D       E |
//                    -----------------------------
//                    |                          |
//                    |                          |
//                    V                          V
//               ---------                   ---------
//               |N O D E|                   |N O D E| <--iter.node (AFTER)
//               ---------                   ---------
//                   |                              | <--iter.pos (AFTER)
//                   |                              |
//                   V                              V
//          ---------                           ---------
//          |N O D E| <--iter.node (BEFORE)     |N O D E|
//          ---------                           ---------
//             | <--iter.pos (BEFORE)
//             |
//             V
//        -----------
//        |KEYSUFFIX|
//        -----------
//             |
//             |
//             V
//          -------
//          |VALUE|
//          -------
//
//***************************************************************************
bool RadixTree::nextNode (Iter &iter) {
  if (isNullPtr(iter.node)) return false;

  Node* node = (Node*) toLocal(iter.node);
  int idx = iter.pos < FANOUT ? iter.pos+1 : 0;

  unsigned keyPos = iter.keyPos;
  Gptr node_gptr, child_gptr;
  node_gptr = iter.node;
  do {
    while (idx >= (int)FANOUT) { // if the current node run out of slot
      uint32_t prefixLen = read32((uint32_t*)&node->prefixLen);
      keyPos -= (prefixLen + 1);
      // pop out current node's parent from the traverse history
      if (iter.nodePath.size()==0)
        return false;
      node_gptr = iter.nodePath.back();
      iter.nodePath.pop_back();

      if (isNullPtr(node_gptr)) return false;
      node = (Node*) toLocal(node_gptr);
      idx = (int)iter.key[keyPos] + 1;

      LOG(debug) << "!! up " << iter.key.substr(0, keyPos) << " idx " << idx << " keypos " << keyPos << " key[keyPos] " << iter.key[keyPos] << "[" << (int)iter.key[keyPos] << "]" ;
    }

    child_gptr = readGptr(&node->child[idx]);
    idx++;
  } while (isNullPtr(child_gptr));

  iter.node = node_gptr;
  iter.pos = idx - 1;
  iter.keyPos = keyPos;
  //LOG(debug) << "!! flag==true: " << iter.key.length() << " iter.keyPos " << iter.keyPos ;
  iter.key = iter.key.substr(0, iter.keyPos);

  LOG(debug) << "!! iter.pos " << iter.pos << " iter.keyPos " << iter.keyPos ;

  LOG(debug) << "!! nextNode before " << iter.key << " iter.key.length() " << iter.key.length() << " iter.keyPos " << iter.keyPos << " ";

  if (iter.keyPos < iter.key.length())
  {
    assert(iter.keyPos == iter.key.length()-1);
    iter.key[iter.keyPos] = (char)(idx - 1);
  }
  else
  {
    iter.key.push_back((char)(idx - 1));
    assert(iter.keyPos == iter.key.length()-1);
  }

  LOG(debug) << "!! nextNode after " << iter.key << " pos [" << (char)iter.pos <<"] " << iter.pos ;

  return true;
}

//***************************************************************************
// Find the next leaf given the current iterator
//***************************************************************************
bool RadixTree::nextLeaf (Iter &iter) {
    LOG(debug) << "NextLeaf on node " <<
        iter.node << " on pos " << iter.pos << "(" << (char)iter.pos << ")" <<
      ": current key(" << iter.key.length() << ") = " << iter.key
      << " with keyPos key[" << iter.keyPos << "]="
      << (iter.keyPos < iter.key.length() ? iter.key[iter.keyPos] : '-');
  //comment Daniel: I don't think we need it because nextNode is shortening the key as well
  //iter.key = iter.key.substr(0, iter.keyPos + 1);

  if (!nextNode(iter))
    return false;
  if (!leftMost(iter))
    return false;
  return true;
}


//************
// GETNEXT   *
//************
Status RadixTree::getNext (char *keyBuf, int &keyBufLen, char *valueBuf, int &valueBufLen, Iter &iter) {
  // if the index is non-unique, try to get the next in the value chain first
  if (!isUnique) {
    Value* currVal = (Value*) toLocal(iter.currValue);
    if (!isNullPtr(currVal->next)) {
      //key buffer size check
      if (keyBufLen < (int)iter.key.length())
        return Status(StatusCode::OUT_OF_RANGE, std::string("getNext FAIL: key buffer NOT large enough."));
      keyBufLen = (int)iter.key.length();
      memcpy(keyBuf, iter.key.c_str(), keyBufLen);

      Value* val = (Value*) toLocal(currVal->next);
      //value buffer size check
      if (valueBufLen < (int)val->len)
        return Status(StatusCode::OUT_OF_RANGE, std::string("getNext FAIL: value buffer NOT large enough."));
      valueBufLen = val->len;
      memcpy(valueBuf, val->data, valueBufLen);

      iter.currValue = currVal->next;
      return Status();
    }
  }

  // get next ----------------------------------------------------------------
  if (!nextLeaf(iter))
    return Status(StatusCode::NOT_FOUND, std::string("getNext FAIL."));

  // return the fetched key value in the returned buffers ---------------------
  if ((iter.endKey != Transaction::OPEN_BOUNDARY) && ((iter.key > iter.endKey) || ((iter.key == iter.endKey) && !iter.endKeyInclusive)))
    return Status(StatusCode::OUT_OF_RANGE, std::string("getNext FAIL: no item in the range."));

  //key buffer size check
  if (keyBufLen < (int)iter.key.length())
    return Status(StatusCode::OUT_OF_RANGE, std::string("getNext FAIL: key buffer NOT large enough."));
  keyBufLen = (int)iter.key.length();
  memcpy(keyBuf, iter.key.c_str(), keyBufLen);
  //std::cout << "!! len " << keyBufLen << " str " << iter.key << std::endl;

  Value* val = getValue(iter);
  //value buffer size check
  if (valueBufLen < (int)val->len)
    return Status(StatusCode::OUT_OF_RANGE, std::string("getNext FAIL: value buffer NOT large enough."));
  valueBufLen = val->len;
  memcpy(valueBuf, val->data, valueBufLen);

  return Status();
}


//***************************************************************************
// Helper function: get the stored value of a leaf slot
//***************************************************************************
Value* RadixTree::getValue (Iter &iter) {
  Node *node = (Node*) toLocal(iter.node);

  Gptr val_gptr;
  if (iter.pos == FANOUT)
    val_gptr = readGptr(&node->val);
  else
    val_gptr = getLeafValue(readGptr(&node->child[iter.pos]));

  Value* val = (Value*) toLocal(val_gptr);

  if (val->isKeySuffix) {
    iter.currValue = readGptr(&val->next);
    val = (Value*) toLocal(val->next);
  }
  else
    iter.currValue = val_gptr;

  return val;
}


//***************************************************************************
// Update the value at the slot the iterator points to
//***************************************************************************
void RadixTree::updateValue (Iter &iter, Gptr &val_gptr) {
  Node *node = (Node*) toLocal(iter.node);

  if (iter.pos == FANOUT) {
    heap->Free(node->val);
    write64((uint64_t*)(&node->val), val_gptr.ToUINT64());
  }
  else {
    Gptr gptr = getLeafValue(readGptr(&node->child[iter.pos]));
    Value* val = (Value*) toLocal(gptr);
    if (val->isKeySuffix) {
      heap->Free(val->next);
      write64((uint64_t*)(&val->next), val_gptr.ToUINT64());
    }
    else {
      heap->Free(gptr);
      write64((uint64_t*)(&node->child[iter.pos]), makeLeafPtr(val_gptr).ToUINT64());
    }
    pmem_invalidate(val, sizeof(Value) + val->len);
  }
}

//************
// UPDATE    *
//************
Status RadixTree::update (const char *key, const int keyLen, const char *value, const int valueLen) {   
  Iter iter;
  if (!find(iter, key, (unsigned)keyLen))
    return Status(StatusCode::NOT_FOUND, std::string("update FAIL: key not found."));

  Gptr val_gptr, next_gptr;
  createValue(false, val_gptr, next_gptr, value, valueLen);

  updateValue(iter, val_gptr);
  return Status();
}


//***************************************************************************
// For unique index, remove anything in the slot the iterator points to.
//
// For non-unique index, remove the specified value in the value chain.
// If no value is specified, remove all values in that slot.
//
// TODO: garbage collection is incomplete, node with no entry in it
// is currently not GCed.
//***************************************************************************
Status RadixTree::remove (Iter &iter, const char *value, const int valueLen) {
  LOG(debug) << "Removing " << std::string(value, valueLen);
  Node *node = (Node*) toLocal(iter.node);
  Gptr val_gptr = (iter.pos == FANOUT) ? readGptr(&node->val) : getLeafValue(readGptr(&node->child[iter.pos]));

  Value* val = (Value*) toLocal(val_gptr);
  Gptr prev_gptr = val_gptr;

  if (valueLen == 0)
    heap->Free(val_gptr);
  else {
    if (std::string(val->data, val->len) == std::string(value, valueLen)) {
      if (iter.pos == FANOUT) {
        //node->val = val->next;
        if (cas64((uint64_t*)&(node->val), val_gptr.ToUINT64(), (val->next).ToUINT64()) != val_gptr.ToUINT64())
          return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
      }
      else {
        //node->child[iter.pos] = val->next;
        if (cas64((uint64_t*)&(node->child[iter.pos]), val_gptr.ToUINT64(), (val->next).ToUINT64()) != val_gptr.ToUINT64())
          return Status(StatusCode::ABORTED, std::string("Insert FAIL: other concurrent transactions have modified the read set."));
      }
      heap->Free(val_gptr);
      pmem_invalidate(val, sizeof(val) + val->len);
      return Status();
    }
  }

  while (!isNullPtr(val->next)) {
    prev_gptr = val_gptr;
    Value* prev = (Value*) toLocal(prev_gptr);
    val_gptr = val->next;
    pmem_invalidate(val, sizeof(val) + val->len);

    val = (Value*) toLocal(val_gptr);
    if (valueLen == 0) {
      prev->next = val->next;
      heap->Free(val_gptr);
    }
    else {
      if (std::string(val->data, val->len) == std::string(value, valueLen)) {
        prev->next = val->next;
        heap->Free(val_gptr);
        pmem_invalidate(val, sizeof(val) + val->len);
        return Status();
      }
    }
  }

  if (valueLen == 0) {
    if (iter.pos == FANOUT)
      node->val = Gptr((uint64_t)0);
    else
      node->child[iter.pos] = Gptr((uint64_t)0);
    return Status();
  }
  else
    return Status(StatusCode::NOT_FOUND, std::string("delete FAIL: value not found."));

  if (val->isKeySuffix)
    heap->Free(val->next);
  heap->Free(val_gptr);

  return Status();
}


//************
// REMOVE    *
//************
Status RadixTree::remove (const char *key, const int keyLen, const char *value, const int valueLen) {
  Iter iter;
  if (!find(iter, key, (unsigned)keyLen))
    return Status(StatusCode::NOT_FOUND, std::string("delete FAIL: key not found."));

  return remove(iter, value, valueLen);
}


Gptr RadixTree::getRoot() {
  return root;
}

//************
// DEBUG     *
//************
void RadixTree::printRoot() {
  Node *node = (Node*) toLocal(root);
  printNode(node, 0);
}

void printIdent (int d) {
  for (int i = 0; i < d; i++)
    std::cout << "    ";
}

void RadixTree::printNode (Node* n, int depth) {
  //std::cout << "*******************************************************************\n";
  printIdent(depth);
  std::cout << "prefix length = " << n->prefixLen << "\n";
  if ((n->prefixLen > 0) && (n->prefixLen <= MAX_INLINE_PREFIX_LEN)) {
    printIdent(depth);
    std::cout << "prefix = ";
    for (unsigned i = 0; i < n->prefixLen; i++)
      std::cout << n->inlinePrefix[i];
    std::cout << "\n";
  }

  printIdent(depth);
  std::cout << "child: ";
  if (!isNullPtr(n->val))
    std::cout << "val:" << n->val << " ";
  for (unsigned i = 0; i < FANOUT; i++) {
    if (!isNullPtr(n->child[i]))
      std::cout << char(i) << ":" << n->child[i] << " ";
  }

  std::cout << "\n";
  printIdent(depth);
  std::cout << "*******************************************************************\n";

  if (n->val.ToUINT64() != 0)
  {
      printIdent(depth + 1);
      std::cout << "SUFFIX NODE ADDRESS = " << n->val << "\n";
      Gptr fetched_child_gptr = n->val;
      while(!isNullPtr(fetched_child_gptr))
      {
          Value *val = (Value*) toLocal(fetched_child_gptr);
          printIdent(depth + 1);
              if (val->isKeySuffix) { // if the position ("k") in the child array points to a key suffix
                  std::cout << "suffix (len " << val->len << ") " << std::string(val->data, val->len) << std::endl;
              }
              else
              {
                  std::cout << "value (len " << val->len << ") " << std::string(val->data, val->len) << std::endl;
              }
          fetched_child_gptr = val->next;
          if (isLeafPtr(fetched_child_gptr))
              fetched_child_gptr = getLeafValue(fetched_child_gptr);
      }
  }
  
  for (unsigned i = 0; i < FANOUT; i++) {
    if (n->child[i].ToUINT64() != 0) {
      if (!isLeafPtr(n->child[i])) {
        // non-leaf node
        printIdent(depth + 1);
        std::cout << "NODE ADDRESS = " << n->child[i] << "\t";
        std::cout << char(i) << " " << i << "\n";
        //std::cout << char(i) << "[" << i << "]" << "\n";
        printNode((Node*) toLocal(n->child[i]), depth + 1);
      }
      else
      {
        printIdent(depth + 1);
        std::cout << "LEAF NODE ADDRESS = " << n->child[i] << "\t";
        std::cout << char(i) << " " << i << "\n";
        // leaf node
        Gptr fetched_child_gptr = getLeafValue(n->child[i]);
        while(!isNullPtr(fetched_child_gptr))
        {
          Value *val = (Value*) toLocal(fetched_child_gptr);
          printIdent(depth + 1);
          if (val->isKeySuffix) { // if the position ("k") in the child array points to a key suffix
            std::cout << "suffix (len " << val->len << ") " << std::string(val->data, val->len) << std::endl;
          }
          else
          {
            std::cout << "value (len " << val->len << ") " << std::string(val->data, val->len) << std::endl;
          }
          fetched_child_gptr = val->next;
          if (isLeafPtr(fetched_child_gptr))
            fetched_child_gptr = getLeafValue(fetched_child_gptr);
        }
      }
    }
  }
}

} // end namespace radixtree
