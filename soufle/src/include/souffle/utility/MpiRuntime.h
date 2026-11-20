/*
 * MPI execution support for generated relation scans.
 * Licensed under the Universal Permissive License v1.0 (see licenses/SOUFFLE-UPL.txt).
 */
#pragma once

#include "souffle/RamTypes.h"
#include <mpi.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <vector>

namespace souffle::mpi_runtime {

inline bool active = false;
inline int worldRank = 0;
inline int worldSize = 1;

[[noreturn]] inline void abort(int code) {
    int initialized = 0, finalized = 0;
    MPI_Initialized(&initialized);
    if (initialized) MPI_Finalized(&finalized);
    if (initialized && !finalized) MPI_Abort(MPI_COMM_WORLD, code);
    std::_Exit(code);
}

inline void check(int error) {
    if (error != MPI_SUCCESS) abort(error);
}

// A generated main owns this session. Embedded users must create a session on
// every rank and call the generated program collectively with identical inputs.
class Session {
    bool ownsMpi = false;

public:
    Session(int& argc, char**& argv) {
        if (active) throw std::logic_error("an MPI execution session is already active");
        int initialized = 0, finalized = 0;
        check(MPI_Initialized(&initialized));
        if (initialized) check(MPI_Finalized(&finalized));
        if (finalized) throw std::logic_error("MPI has already been finalized");
        if (!initialized) {
            check(MPI_Init(&argc, &argv));
            ownsMpi = true;
        }
        check(MPI_Comm_rank(MPI_COMM_WORLD, &worldRank));
        check(MPI_Comm_size(MPI_COMM_WORLD, &worldSize));
        // Existing generated I/O handlers call exit(1). Abort peers rather than
        // leaving them blocked forever in a collective if one rank exits early.
        static const bool registered = [] {
            return std::atexit([] { if (active) abort(1); }) == 0;
        }();
        if (!registered) abort(1);
        active = true;
    }
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    ~Session() {
        if (std::uncaught_exceptions() != 0) abort(1);
        if (ownsMpi) check(MPI_Finalize());
        active = false;
    }
};

inline std::size_t rank() {
    if (!active) throw std::logic_error("MPI generated code requires an mpi_runtime::Session");
    return static_cast<std::size_t>(worldRank);
}

inline std::size_t size() {
    if (!active) throw std::logic_error("MPI generated code requires an mpi_runtime::Session");
    return static_cast<std::size_t>(worldSize);
}

template <class TupleType>
bool owns(const TupleType& tuple) {
    if (size() == 1) return true;
    std::uint64_t hash = 14695981039346656037ULL;
    for (auto cell : tuple) {
        hash ^= static_cast<std::uint64_t>(cell);
        hash *= 1099511628211ULL;
    }
    // Mix high and low bits before reducing to a rank, including for powers of 2.
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    return hash % size() == rank();
}

// Exchange only successful local inserts, then union them into every replica.
// Bounded broadcasts avoid MPI's int count/displacement limit and a second
// allocation proportional to the global result size. Tuples travel as domain
// cells, never as implementation-dependent B-tree or Tuple object layouts.
template <std::size_t Arity, class Relation>
void synchronize(Relation& relation, const std::vector<RamDomain>& added) {
    static_assert(Arity > 0 && Arity <= 6);
    if (size() == 1) return;
    constexpr std::size_t chunkCells = (65536 / Arity) * Arity;
    std::vector<RamDomain> buffer(chunkCells);
    auto context = relation.createContext();
    for (int source = 0; source < worldSize; ++source) {
        std::uint64_t count = worldRank == source ? added.size() : 0;
        check(MPI_Bcast(&count, 1, MPI_UINT64_T, source, MPI_COMM_WORLD));
        if (count % Arity != 0) abort(1);
        for (std::uint64_t offset = 0; offset < count;) {
            const auto cells = static_cast<std::size_t>(
                    std::min<std::uint64_t>(chunkCells, count - offset));
            if (worldRank == source) std::copy_n(added.data() + offset, cells, buffer.data());
            check(MPI_Bcast(buffer.data(), static_cast<int>(cells * sizeof(RamDomain)),
                    MPI_BYTE, source, MPI_COMM_WORLD));
            if (worldRank != source) {
                for (std::size_t i = 0; i < cells; i += Arity) {
                    Tuple<RamDomain, Arity> tuple;
                    for (std::size_t j = 0; j < Arity; ++j) tuple[j] = buffer[i + j];
                    relation.insert(tuple, context);
                }
            }
            offset += cells;
        }
    }
}

}  // namespace souffle::mpi_runtime
