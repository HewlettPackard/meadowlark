#ifndef FAME

// multi-thread version

#include "acked_counter_generator.h"
#include <atomic>

namespace ycsbc {

std::atomic<uint64_t> AckedCounterGenerator::limit_(0);
std::atomic<uint64_t> AckedCounterGenerator::counter_(0);

} // ycsbc

#else // FAME

#endif // FAME
