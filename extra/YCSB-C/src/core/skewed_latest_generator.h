//
//  skewed_latest_generator.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/9/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_SKEWED_LATEST_GENERATOR_H_
#define YCSB_C_SKEWED_LATEST_GENERATOR_H_

#include "generator.h"

#include <cstdint>
#include <mutex>
#include "counter_generator.h"
#include "zipfian_generator.h"

namespace ycsbc {

class SkewedLatestGenerator : public Generator<uint64_t> {
 public:
    SkewedLatestGenerator(Generator<uint64_t> *counter) :
      basis_(counter), zipfian_(basis_->Last()) {
    Next();
  }

  uint64_t Next();
  uint64_t Last() { return last_; }
 private:
  Generator<uint64_t> *basis_;
  ZipfianGenerator zipfian_;
  uint64_t last_;
  std::mutex mutex_;
};

// NOTE: current implementation of SkewedLatestGenerator NOT thread-safe when Next(uint64_t num) is used
// (e.g., by SkewedLatestGenerator )
inline uint64_t SkewedLatestGenerator::Next() {
  std::lock_guard<std::mutex> guard(mutex_);
  uint64_t max = basis_->Last();
  return last_ = max - zipfian_.Next(max);
}

} // ycsbc

#endif // YCSB_C_SKEWED_LATEST_GENERATOR_H_
