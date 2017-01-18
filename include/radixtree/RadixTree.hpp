#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <vector>
#include <cstring>
#include <assert.h>
#include <iostream>

#include "radixtree/radixtree_libpmem.h"
#include "radixtree/radixtree_fam_atomic.h"
#include "radixtree/status.h"
#include "radixtree/log.h"

#include "nvmm/memory_manager.h"
#include "nvmm/heap.h"

namespace radixtree {

nvmm::PoolId const RADIXTREE_POOL_ID = 1;
    
class RadixTree {    
  public:
    static const unsigned FANOUT = 256; // don't change
    static const unsigned MAX_INLINE_PREFIX_LEN = 8;

    // Currently, the most significant bit (MSB) denotes whether a global pointer
    // points to a child node or a leaf value
    //
    // TODO: After alignment issue is solved for GlobalPtr, this mask should
    // switch to the least significant bit (LSB), i.e., 0x0000000000000001
    static const uint64_t LEAF_BIT_MASK = 0x0000000000000001;

    typedef nvmm::GlobalPtr Gptr;
    typedef nvmm::MemoryManager Mmgr;
    typedef nvmm::Heap Heap;

    //*************************************************************************
    // Structure Definition                                                   *
    //*************************************************************************

    //***************************
    // Radix Tree Node          *
    //***************************
    //*************************************************************************
    //                   ---
    //                   |P|
    //                   |R|
    //                   |E|
    //                   |F|
    //                   |I|
    //                   |X|
    //     ---------------------------------
    //     | PARENT | VAL | CHILD POINTERS |
    //     ---------------------------------
    //                 |       |     |   |
    //                 |       V     V   V
    //                 V
    //             ---------
    //             | Value |
    //             ---------
    //
    //*************************************************************************
    //
    //  Example Key-values: (kim, awesome), (kimberly, cool)
    //
    //           -----------------
    //           |    |k|        |
    //           -----------------
    //                 |
    //                 |
    //                 V
    //                 _
    //                |i|
    //          v     |m|
    //         -------------------  
    //         | |         |b|   |
    //         -------------------  
    //          |           |
    //          |           |
    //          V           V
    //      ---------     ------
    //      |awesome|     |erly|
    //      ---------     ------
    //                      |
    //                      |
    //                      V
    //                   ------
    //                   |cool|
    //                   ------
    //
    //*************************************************************************
    // No external prefix allowed in current implementation
    typedef struct Node {
      public:
        uint32_t prefixLen;
        char inlinePrefix[MAX_INLINE_PREFIX_LEN];
        Gptr val; // position FANOUT
        Gptr child[FANOUT];
    } Node;

    //***************************
    // Value or KeySuffix       *
    //***************************
    typedef struct Value {
        uint32_t isKeySuffix;
        Gptr next;
        uint32_t len;
        char data[0];
    } Value;

    //***************************
    // Iterator                 *
    //***************************
    typedef struct Iter {
        Gptr node; // current node the iterator points to
        uint32_t pos; // current position in the child array
        std::string key; // current key string
        uint32_t keyPos; // current char position in the key string

        Gptr currValue; // current value object the iterator points to

        // for range query boundaries
        std::string beginKey;
        bool beginKeyInclusive;
        std::string endKey;
        bool endKeyInclusive;

        std::vector<Gptr> nodePath; // node travaersal history for range query
    } Iter;

  public:
    //***************************
    // CONSTRUCTORS             *
    //***************************
    inline void init();
    RadixTree(bool u = true);
    RadixTree(Gptr r, bool u = true);
    RadixTree(Mmgr *m, Heap *h, bool u = true);
    RadixTree(Gptr r, Mmgr *m, Heap *h, bool u = true);
    virtual ~RadixTree();

    //***************************
    // PUBLIC INTERFACE         *
    //***************************
    Status insert (const char *key, const int keyLen, const char *value, const int valueLen);
    Status scan (char *keyBuf, int &keyBufLen, char *valueBuf, int &valueBufLen, Iter &iter, const char *beginKey, const int beginKeyLen, const bool beginKeyInclusive, const char *endKey, const int endKeyLen, const bool endKeyInclusive);
    Status getNext (char *keyBuf, int &keyBufLen, char *valueBuf, int &valueBufLen, Iter &iter);
    Status update (const char *key, const int keyLen, const char *value, const int valueLen);
    Status remove (const char *key, const int keyLen, const char *value = NULL, const int valueLen = 0);

    Gptr getRoot();


    //***************************
    // COMMON HELPERS           *
    //***************************
    // convert global (FAM) pointer to local (DRAM) pointer
    void* toLocal(const Gptr &gptr);
    // check whether the global pointer is null
    inline bool isNullPtr(const Gptr &gptr);

    // convert between child pointer and leaf pointer
    inline Gptr makeLeafPtr(const Gptr &gptr);
    inline bool isLeafPtr(const Gptr &gptr);
    Gptr getLeafValue(const Gptr &gptr);


    //***************************
    // INSERT HELPERS           *
    //***************************
    Value* createValue (const bool isKsuf, Gptr &val_gptr, const Gptr &next_gptr, const char *str, const unsigned len);
    Node* createNode (Gptr &node_gptr, const char *prefix, const uint32_t prefixLen);

    Status installKeyValue (const bool isAtomic, Gptr &ksuf_gptr, Gptr &val_gptr, Node *node, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen);

    Status splitPrefix (unsigned breakPos, Gptr &parent_gptr, int parent_pos, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen);
    Status splitSuffix (unsigned breakPos, Gptr &parent_gptr, int parent_pos, Gptr &old_child_gptr, Value *ksuf, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen);

    Status appendValue (Gptr &val_gptr, const char *value, const unsigned valueLen);

    // Recursive call for insert
    Status insertToNode (Gptr &parent_gptr, int parent_pos, const char *key, const unsigned keyLen, const char *value, const unsigned valueLen);


    //***************************
    // POINT QUERY HELPERS      *
    //***************************
    bool findRecursive (Iter &iter);
    bool find (Iter &iter, const char *key, const unsigned keyLen);


    //***************************
    // RANGE QUERY HELPERS      *
    //***************************
    bool leftMost (Iter &iter);
    bool lowerBoundRecursive (Iter &iter, bool isInclusive);
    bool lowerBound (Iter &iter, const char *key, const unsigned keyLen, bool isInclusive);

    bool nextNode (Iter &iter);
    bool nextLeaf (Iter &iter);


    //***************************
    // UPDATE HELPERS           *
    //***************************
    Value* getValue (Iter &iter);
    void updateValue (Iter &iter, Gptr &val_gptr);


    //***************************
    // DELETE HELPERS           *
    //***************************
    Status remove(Iter &iter, const char *value, const int valueLen);

    //***************
    // DEBUG        *
    //***************
    void printRoot();
    void printNode (Node* n, int depth);


  private:
    RadixTree(const RadixTree&);              // disable copying
    RadixTree& operator=(const RadixTree&);   // disable assignment

  private:
    Mmgr *mmgr;
    Heap *heap;
    Gptr root;
    bool isUnique; // whether the index supports non-unique keys
    bool closeHeap; // whether we should close the heap (true when init() is called)
};

}


#endif /* RADIX_TREE_HPP */
