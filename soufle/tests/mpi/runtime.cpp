/* Licensed under the Universal Permissive License v1.0. */
#include "souffle/utility/MpiRuntime.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>

using namespace souffle;

struct Relation {
    std::set<Tuple<RamDomain, 3>> rows;
    int createContext() const { return 0; }
    bool insert(const Tuple<RamDomain, 3>& tuple, int&) { return rows.insert(tuple).second; }
};

int main(int argc, char** argv) {
    bool rejected = false;
    try { mpi_runtime::rank(); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    const std::string mode = argc > 1 ? argv[1] : "owned";
    const bool external = mode == "external";
    if (external) MPI_Init(&argc, &argv);
    {
        mpi_runtime::Session session(argc, argv);
        if (mode == "exit" && mpi_runtime::rank() == 1) std::exit(1);
        if (mode == "exception" && mpi_runtime::rank() == 1) {
            throw std::runtime_error("test rank failure");
        }
        Relation relation;
        std::vector<RamDomain> added;
        auto context = relation.createContext();
        // More than one broadcast chunk per rank, including negative values and
        // values beyond 32 bits. Every rank contributes distinct tuples.
        for (RamDomain i = 0; i < 40000; ++i) {
            Tuple<RamDomain, 3> tuple{{static_cast<RamDomain>(mpi_runtime::rank()),
                    i, -((RamDomain(1) << 40) + i)}};
            relation.insert(tuple, context);
            added.insert(added.end(), tuple.begin(), tuple.end());
        }
        mpi_runtime::synchronize<3>(relation, added);
        assert(relation.rows.size() == 40000 * mpi_runtime::size());
        unsigned long long owned = 0, totalOwned = 0;
        for (const auto& tuple : relation.rows) owned += mpi_runtime::owns(tuple);
        MPI_Allreduce(&owned, &totalOwned, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
        assert(owned > 0 && totalOwned == relation.rows.size());
        for (std::size_t rank = 0; rank < mpi_runtime::size(); ++rank) {
            assert(relation.rows.count({{static_cast<RamDomain>(rank), 39999,
                    -((RamDomain(1) << 40) + 39999)}}));
        }
        // Duplicate exchange and ranks contributing no rows must also work.
        mpi_runtime::synchronize<3>(relation, added);
        mpi_runtime::synchronize<3>(relation, std::vector<RamDomain>{});
        assert(relation.rows.size() == 40000 * mpi_runtime::size());
        if (mpi_runtime::rank() == 0) std::cout << "PASS MPI chunked exchange and replicas\n";
    }
    int finalized = 0;
    MPI_Finalized(&finalized);
    assert(finalized == !external);
    if (external) MPI_Finalize();
}
