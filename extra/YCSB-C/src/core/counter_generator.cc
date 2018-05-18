#ifndef FAME

// multi-thread version

#include "counter_generator.h"
#include <atomic>

namespace ycsbc {

std::atomic<uint64_t> CounterGenerator::counter_(0);

} // ycsbc

#else // FAME

#endif // FAME
