// rel.h - basic types shared by every back-end (our code)
//
// A fact of a two-column relation R(a, b) is stored as ONE 64-bit number:
//     t = (a << 32) | b
// so sorting the numbers sorts the facts by column 0 (then column 1), and a whole
// relation is just an array of uint64_t. "Keyed by column 0" = sorted array.
#ifndef REL_H
#define REL_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>

typedef uint64_t tup;

#ifndef HD
#if defined(__CUDACC__)
#define HD __host__ __device__
#else
#define HD
#endif
#endif

HD inline tup make_tup(uint32_t a, uint32_t b) { return ((uint64_t)a << 32) | b; }
HD inline uint32_t col0(tup t) { return (uint32_t)(t >> 32); }
HD inline uint32_t col1(tup t) { return (uint32_t)t; }
HD inline tup flip_tup(tup t) { return (t << 32) | (t >> 32); }

// order-independent hash of a set of facts: sum of a mixed value per fact.
// Two relations with the same facts have the same (count, hash) on every back-end.
HD inline uint64_t mix64(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// std::vector fills new memory with zeros, in ONE thread. For a 250 MB result array that we are
// about to overwrite in parallel anyway, that serial memset was a big part of each merge
// (found with perf). This allocator leaves new elements uninitialized.
template <typename T>
struct uninit_alloc : std::allocator<T> {
    template <typename U> struct rebind { typedef uninit_alloc<U> other; };
    uninit_alloc() = default;
    template <typename U> uninit_alloc(const uninit_alloc<U> &) {}
    template <typename U> void construct(U *p) noexcept { ::new ((void *)p) U; }
    template <typename U, typename... Args> void construct(U *p, Args &&...args)
    {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }
};
typedef std::vector<tup, uninit_alloc<tup>> HostRel;   // relation type of the CPU back-ends

#define DIE(...) do { fprintf(stderr, "error: " __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while (0)

inline double now_sec()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// time spent in each kind of operator (filled by the back-ends)
struct Prof {
    double join = 0;    // join: matching Δ facts with the other relation
    double sort = 0;    // sort + unique (dedup) of candidates / temporaries, flips, filters
    double diff = 0;    // set difference with the full relation + merge into it
    double comm = 0;    // MPI exchange / GPU transfers
    long iters = 0;     // fixpoint rounds
};

#endif
