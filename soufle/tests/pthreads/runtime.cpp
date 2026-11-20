#include "souffle/utility/PthreadRuntime.h"
#include "souffle/utility/ParallelUtil.h"
#include <cassert>
#include <numeric>
#include <set>
#include <iostream>
int main() {
    using namespace souffle::pthread_runtime;
    configure(4);
    std::vector<size_t> parts(2000); std::iota(parts.begin(), parts.end(), 0);
    std::atomic<size_t> sum{0};
    pthread_barrier_t barrier; pthread_barrier_init(&barrier, nullptr, 4);
    parallelFor(parts, [&](size_t i) {
        if (i < 4) pthread_barrier_wait(&barrier);
        assert(workerLane < 4); sum.fetch_add(i);
    });
    pthread_barrier_destroy(&barrier);
    assert(sum == 1999000);
    bool caught = false;
    try { parallelFor(parts, [](size_t i) { if (i == 10) throw std::runtime_error("test"); }); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);
    sum = 0;
    parallelFor(parts, [&](size_t) { ++sum; }); assert(sum == parts.size());
    configure(4, false);
    auto caller=pthread_self();
    parallelFor(parts, [&](size_t) { assert(pthread_equal(caller,pthread_self())); });
    configure(1); parallelFor(parts, [&](size_t) { assert(pthread_equal(caller,pthread_self())); });
    configure(2); sum=0; parallelFor(parts, [&](size_t) { ++sum; }); assert(sum == parts.size());
    std::cout << "PASS pthread worker execution, synchronization, exception recovery, sequential mode, and resizing\n";
}
