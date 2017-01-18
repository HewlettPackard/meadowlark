//************************************************
// RadixTree example
//************************************************
#include "radixtree/status.h"
#include "radixtree/RadixTree.hpp"
#include "radixtree/Transaction.hpp"

#include <iostream>

using namespace radixtree;

static const unsigned BUFFER_SIZE = 1024;
static const int KEY_VALUE_COUNT = 3;

int main (int argc, char** argv) {
    // sample data
    std::string keys[KEY_VALUE_COUNT]={"key0", "key1", "key2"};
    std::string values[KEY_VALUE_COUNT]={"value0", "value1", "value2"};

    // buffers to store scan/lookup results
    char keyBuf[BUFFER_SIZE];
    int keyBufLen;
    char valueBuf[BUFFER_SIZE];
    int valueBufLen;

    // return code
    Status ret;

    // iterator for scan/lookup
    RadixTree::Iter iter;    
        
    // create a new radix tree
    RadixTree *index = new RadixTree();
    
    // insert three key-value pairs
    std::cout << "Inserting key value pairs:" << std::endl;
    for(int i=0; i<KEY_VALUE_COUNT; i++) {
        ret = index->insert(keys[i].c_str(), (int)keys[i].length(), values[i].c_str(), (int)values[i].length());
        assert(ret.ok()==true);
        std::cout << "key: " << keys[i] << std::endl;
        std::cout << "val: " << values[i] << std::endl;
    }

    // point lookups
    std::cout << "Point lookups:" << std::endl;
    for(int i=0; i<KEY_VALUE_COUNT; i++) {
        ret = index->scan(keyBuf, keyBufLen, valueBuf, valueBufLen, iter, keys[i].c_str(), (int)keys[i].length(), true, keys[i].c_str(), (int)keys[i].length(), true);
        assert(ret.ok()==true);
        std::cout << "key: " << keys[i] << std::endl;
        std::cout << "val: " << values[i] << std::endl;
    }

    // update keys[0]'s value with keys[2]'s value
    std::cout << "Update the value of an existing key:" << std::endl;
    ret = index->update(keys[0].c_str(), (int)keys[0].length(), keys[2].c_str(), (int)keys[2].length());
    assert(ret.ok()==true);
    
    // remove keys[1]
    std::cout << "Remove a key value pair:" << std::endl;
    ret = index->remove(keys[1].c_str(), (int)keys[1].length());
    assert(ret.ok()==true);
    
    // range scan
    std::cout << "Range scan:" << std::endl;
    ret = index->scan(keyBuf, keyBufLen, valueBuf, valueBufLen, iter, keys[0].c_str(), (int)keys[0].length(), true, keys[2].c_str(), (int)keys[2].length(), true);
    assert(ret.ok()==true);
    while (index->getNext(keyBuf, keyBufLen, valueBuf, valueBufLen, iter).ok()) {
        std::string key(keyBuf, keyBufLen);
        std::string val(valueBuf, valueBufLen);
        std::cout << "key: " << key << std::endl;
        std::cout << "val: " << val << std::endl;

        // reset both lengths
        keyBufLen = BUFFER_SIZE;
        valueBufLen = BUFFER_SIZE;    
    }
    
    // delete the radix tree
    delete index;
}
