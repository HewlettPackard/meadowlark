//
//  acked_counter_generator.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/9/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_ACKED_COUNTER_GENERATOR_H_
#define YCSB_C_ACKED_COUNTER_GENERATOR_H_

#ifndef FAME

// multi-thread version

#include "generator.h"

#include <cstdint>
#include <atomic>
#include <mutex>
#include <vector>
#include <iostream>

namespace ycsbc {

class AckedCounterGenerator : public Generator<uint64_t> {
 public:
    AckedCounterGenerator() {
        window_=std::vector<bool>(WINDOW_SIZE, false);
    }

    static void InitShared(uint64_t start) {
        counter_.store(start);
        limit_.store(start-1);
    }
    // void Set(uint64_t start) {
    //     counter_.store(start);
    // }
    uint64_t Next() {
        uint64_t ret = counter_.fetch_add(1);
        //std::cout << "Next " << ret << std::endl;
        return ret;
    }
    uint64_t Last() {
        uint64_t ret = limit_.load();
        //std::cout << "Last " << ret << std::endl;
        return ret;
    }

    void Ack(uint64_t value) {
        uint64_t currentSlot = (value & WINDOW_MASK);
        if(window_[currentSlot]) {
            std::cout << "Too many unacknowledged insertion keys" << std::endl;
            exit(1);
        }

        window_[currentSlot]=true;


        std::lock_guard<std::mutex> guard(mutex_);

        uint64_t beforeFirstSlot = (limit_ & WINDOW_MASK);
        uint64_t index;
        for(index=limit_+1; (index& WINDOW_MASK) !=beforeFirstSlot; index++) {
            uint64_t slot = (index & WINDOW_MASK);
            if(!window_[slot]) break;
            window_[slot]=false;
        }
        limit_ = index-1;
    }
private:
    static uint64_t const WINDOW_SIZE = 1UL<<32;
    static uint64_t const WINDOW_MASK = WINDOW_SIZE-1;
    static std::atomic<uint64_t> counter_;
    static std::atomic<uint64_t> limit_;
    std::vector<bool> window_;
    std::mutex mutex_;
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

class AckedCounterGenerator : public Generator<uint64_t> {
 public:
    AckedCounterGenerator()
        : mm_(nullptr), region_(nullptr), base_(nullptr),
          counter_(nullptr), lock_(nullptr), limit_(nullptr), window_(nullptr) {
        Init();
    }
    ~AckedCounterGenerator() {
        Final();
    }
    uint64_t Next() {
        uint64_t ret = fam_atomic_u64_fetch_and_add(counter_, 1);
        //std::cout << "Next " << ret << std::endl;
        return ret;
    }
    uint64_t Last() {
        uint64_t ret = fam_atomic_u64_read(limit_);
        //std::cout << "Last " << ret << std::endl;
        return ret;
    }

    void Ack(uint64_t value) {
        if(CheckSlot(value)) {
            std::cout << "Too many unacknowledged insertion keys" << std::endl;
            exit(1);
        }

        MarkSlot(value);

        Lock();
        uint64_t limit = fam_atomic_u64_read(limit_);
        uint64_t beforeFirstSlot = (limit & WINDOW_MASK);
        uint64_t index = limit + 1;
        for(; (index& WINDOW_MASK) !=beforeFirstSlot; index++) {
            if(!CheckSlot(index)) break;
            UnmarkSlot(index);
        }
        fam_atomic_u64_write(limit_, index-1);
        Unlock();
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

        char *base, *cur;
        ret = region->Open(O_RDWR);
        assert(ret==nvmm::NO_ERROR);
        ret = region->Map(NULL, size_, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&base);
        assert(ret==nvmm::NO_ERROR);

        cur = base;
        nvmm_fam_spinlock *lock = (nvmm_fam_spinlock*)cur;
        lock->init();

        cur+=sizeof(nvmm_fam_spinlock);
        uint64_t *counter = (uint64_t*)cur;
        fam_atomic_u64_write(counter, start);

        cur+=sizeof(uint64_t);
        uint64_t *limit = (uint64_t*)cur;
        fam_atomic_u64_write(limit, start-1);

        cur+=sizeof(uint64_t);
        uint64_t *window = (uint64_t*)cur;
        for(uint64_t i=0; i<WINDOW_SIZE; i++) {
            window[i]=false;
        }

        fam_persist(base, sizeof(nvmm_fam_spinlock)+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(uint64_t)*WINDOW_SIZE);

        ret = region->Unmap((void*)base, size_);
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
        ret = region_->Map(NULL, size_, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&base_);
        assert(ret==nvmm::NO_ERROR);

        char *cur = base_;
        lock_ = (nvmm_fam_spinlock*)cur;

        cur+=sizeof(nvmm_fam_spinlock);
        counter_ = (uint64_t*)cur;

        cur+=sizeof(uint64_t);
        limit_ = (uint64_t*)cur;

        cur+=sizeof(uint64_t);
        window_ = (uint64_t*)cur;
    }

    void Final() {
        nvmm::ErrorCode ret = region_->Unmap((void*)base_, size_);
        assert(ret == nvmm::NO_ERROR);
        ret = region_->Close();
        assert(ret == nvmm::NO_ERROR);
        delete region_;
    }

private:
    static nvmm::PoolId const region_id_ = 4;
    static size_t const size_ = 128*1024*1024; // 128MB
    static uint64_t const WINDOW_SIZE = 1UL<<20;
    static uint64_t const WINDOW_MASK = WINDOW_SIZE-1;

    nvmm::MemoryManager *mm_;
    nvmm::Region *region_;

    char *base_;

    nvmm_fam_spinlock* lock_;
    uint64_t *counter_; // fam atomics
    uint64_t *limit_; // fam lock
    uint64_t *window_; // fam lock

    inline void Lock()
    {
        lock_->lock();
    }

    inline void Unlock()
    {
        lock_->unlock();
    }

    inline void MarkSlot(uint64_t value)
    {
        uint64_t *slot = window_;
        uint64_t currentSlot = (value & WINDOW_MASK);
        slot+=currentSlot;
        fam_atomic_u64_write(slot, 1);
    }

    inline void UnmarkSlot(uint64_t value)
    {
        uint64_t *slot = window_;
        uint64_t currentSlot = (value & WINDOW_MASK);
        slot+=currentSlot;
        fam_atomic_u64_write(slot, 0);
    }

    inline bool CheckSlot(uint64_t value)
    {
        uint64_t *slot = window_;
        uint64_t currentSlot = (value & WINDOW_MASK);
        slot+=currentSlot;
        return fam_atomic_u64_read(slot)==1;
    }
};

} // ycsbc

#endif // FAME

#endif // YCSB_C_ACKED_COUNTER_GENERATOR_H_
