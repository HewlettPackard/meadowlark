//
//  counter_generator.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/9/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_COUNTER_GENERATOR_H_
#define YCSB_C_COUNTER_GENERATOR_H_

#ifndef FAME

// multi-thread version

#include "generator.h"

#include <cstdint>
#include <atomic>

namespace ycsbc {

class CounterGenerator : public Generator<uint64_t> {
 public:
    CounterGenerator() { }

    static void InitShared(uint64_t start) {
        counter_.store(start);
    }
    uint64_t Next() {
        return counter_.fetch_add(1);
    }
    uint64_t Last() {
        return counter_.load() - 1;
    }
    void Set(uint64_t start) {
        counter_.store(start);
    }
private:
    static std::atomic<uint64_t> counter_;
};

} // ycsbc

#else // FAME

// FAM (multi-process/multi-node) version

#include "generator.h"

#include <cstdint>
#include <nvmm/fam.h>
#include <nvmm/memory_manager.h>
#include <nvmm/region.h>

namespace ycsbc {

class CounterGenerator : public Generator<uint64_t> {
 public:
    CounterGenerator()
        : mm_(nullptr), region_(nullptr), counter_(nullptr) {
        Init();
    }
    ~CounterGenerator() {
        Final();
    }
    uint64_t Next() {
        return fam_atomic_u64_fetch_and_add(counter_, 1);
    }
    uint64_t Last() {
        return fam_atomic_u64_read(counter_) - 1;
    }

    // NOT thread-safe/process-safe/node-safe!
    static void InitShared(uint64_t start) {
        nvmm::MemoryManager* mm = nvmm::MemoryManager::GetInstance();
        nvmm::Region* region = mm->FindRegion(region_id_);
        nvmm::ErrorCode ret;
        if(!region) {
            ret = mm->CreateRegion(region_id_, size_);
            assert(ret==nvmm::NO_ERROR);
            region = mm->FindRegion(region_id_);
        }
        assert(region);
        uint64_t *counter;
        ret = region->Open(O_RDWR);
        assert(ret==nvmm::NO_ERROR);
        ret = region->Map(NULL, size_, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&counter);
        assert(ret==nvmm::NO_ERROR);

        fam_atomic_u64_write(counter, start);

        ret = region->Unmap((void*)counter, size_);
        assert(ret == nvmm::NO_ERROR);
        ret = region->Close();
        assert(ret == nvmm::NO_ERROR);
        delete region;
    }

    void Init() {
        mm_ = nvmm::MemoryManager::GetInstance();
        region_ = mm_->FindRegion(region_id_);
        assert(region_);
        nvmm::ErrorCode ret = region_->Open(O_RDWR);
        assert(ret==nvmm::NO_ERROR);
        ret = region_->Map(NULL, size_, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&counter_);
        assert(ret==nvmm::NO_ERROR);
    }
    void Final() {
        nvmm::ErrorCode ret = region_->Unmap((void*)counter_, size_);
        assert(ret == nvmm::NO_ERROR);
        ret = region_->Close();
        assert(ret == nvmm::NO_ERROR);
        delete region_;
    }

private:
    static nvmm::PoolId const region_id_ = 4;
    static size_t const size_ = 128*1024*1024; // 128MB
    nvmm::MemoryManager *mm_;
    nvmm::Region *region_;
    uint64_t *counter_;
};

} // ycsbc

#endif // FAME

#endif // YCSB_C_COUNTER_GENERATOR_H_
