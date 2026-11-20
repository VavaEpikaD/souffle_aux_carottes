// mpi_backend.h - Person B: MPI (distributed memory) back-end of the 6 operators (our code)
//
// Every relation is HASH-PARTITIONED by its key column (column 0):
//      fact (k, x) lives on rank owner(k) = hash(k) mod P
// so for a join a(k,x) ⋈ b(k,y) both sides of every key are on the same rank: the join is
// purely LOCAL (the sequential join of host_backend.h). Its output (x,y) belongs to the rank
// that owns x, so `key` and `diff_merge` first EXCHANGE the facts with MPI_Alltoallv, then
// sort / dedup / minus / merge locally. The loop stops when the global sum of |Δ| (MPI_Allreduce)
// is 0. That is v1.
//
// Per-round cost we measure: local work (join, sort, diff) vs communication (comm).
// Next steps (not in v1): degree-aware partitioning for hub keys, batching several local
// rounds per exchange, one OpenMP team per rank.
#ifndef MPI_BACKEND_H
#define MPI_BACKEND_H

#include <mpi.h>
#include "../engine/host_backend.h"

struct MpiBackend : HostBackend {
    int rank = 0, nprocs = 1, version = 1;
    uint64_t bytes_sent = 0;

    const char *name() { return "mpi"; }
    int workers() { return nprocs; }
    bool is_root() { return rank == 0; }
    void barrier() { MPI_Barrier(MPI_COMM_WORLD); }

    int owner(tup t) const { return (int)(mix64(col0(t)) % (uint64_t)nprocs); }

    // every rank reads the input files and keeps only its own part
    Rel load(const std::vector<tup> &host)
    {
        Rel r;
        for (tup t : host)
            if (owner(t) == rank) r.push_back(t);
        return HostBackend::key(std::move(r));
    }

    // ---- send every fact to the rank that owns its column 0 ----
    Rel exchange(Rel &&r)
    {
        double t0 = now_sec();
        std::vector<int> scount(nprocs, 0), rcount(nprocs), sdisp(nprocs), rdisp(nprocs);
        for (tup t : r) scount[owner(t)]++;
        sdisp[0] = 0;
        for (int p = 1; p < nprocs; p++) sdisp[p] = sdisp[p - 1] + scount[p - 1];
        Rel sendbuf(r.size());
        std::vector<int> pos(sdisp);
        for (tup t : r) sendbuf[pos[owner(t)]++] = t;
        Rel().swap(r);
        MPI_Alltoall(scount.data(), 1, MPI_INT, rcount.data(), 1, MPI_INT, MPI_COMM_WORLD);
        rdisp[0] = 0;
        for (int p = 1; p < nprocs; p++) rdisp[p] = rdisp[p - 1] + rcount[p - 1];
        Rel recvbuf((size_t)rdisp[nprocs - 1] + rcount[nprocs - 1]);
        MPI_Alltoallv(sendbuf.data(), scount.data(), sdisp.data(), MPI_UINT64_T,
                      recvbuf.data(), rcount.data(), rdisp.data(), MPI_UINT64_T, MPI_COMM_WORLD);
        bytes_sent += (sendbuf.size() - scount[rank]) * sizeof(tup);
        prof.comm += now_sec() - t0;
        return recvbuf;
    }

    Rel key(Rel &&r) { return HostBackend::key(exchange(std::move(r))); }

    Rel diff_merge(Rel &&cand, Full &full) { return HostBackend::diff_merge(exchange(std::move(cand)), full); }

    // global size: sum over ranks
    uint64_t size(const Rel &r)
    {
        double t0 = now_sec();
        uint64_t local = r.size(), global = 0;
        MPI_Allreduce(&local, &global, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
        prof.comm += now_sec() - t0;
        return global;
    }

    // size + hash are sums, so they can be added over ranks; CSV only for small outputs
    Result result(const std::string &name, const Rel &r, bool flipped_rel)
    {
        Result res;
        res.name = name;
        uint64_t local[2] = {r.size(), 0}, global[2];
        for (tup t : r) local[1] += mix64(flipped_rel ? flip_tup(t) : t);
        MPI_Allreduce(local, global, 2, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
        res.size = global[0];
        res.hash = global[1];
        if (!outdir.empty()) {
            int n = (int)r.size();
            std::vector<int> cnt(nprocs), disp(nprocs);
            MPI_Gather(&n, 1, MPI_INT, cnt.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
            Rel all;
            if (rank == 0) {
                disp[0] = 0;
                for (int p = 1; p < nprocs; p++) disp[p] = disp[p - 1] + cnt[p - 1];
                all.resize((size_t)disp[nprocs - 1] + cnt[nprocs - 1]);
            }
            MPI_Gatherv(r.data(), n, MPI_UINT64_T, all.data(), cnt.data(), disp.data(), MPI_UINT64_T, 0,
                        MPI_COMM_WORLD);
            if (rank == 0) {
                std::vector<tup> v(all.begin(), all.end());
                write_csv(outdir, name, flipped_rel ? flipped(v) : v);
            }
        }
        return res;
    }
};

#endif
