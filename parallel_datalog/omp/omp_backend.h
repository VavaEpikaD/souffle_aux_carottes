// omp_backend.h - Person A: OpenMP (shared memory) back-end of the 6 operators (our code)
//
// Same relations as the sequential back-end (sorted std::vector<tup>), different operators:
//
//   v1  OPT join:  Δ is cut into many pieces at key-group borders, pieces are joined in
//                  parallel with schedule(dynamic) (hub keys make pieces very uneven),
//                  every thread writes its own buffer, buffers are concatenated in parallel.
//       OPT dedup / minus / merge: parallel sort (__gnu_parallel::sort), parallel unique, and
//                  OUR OWN parallel set difference and merge (par_difference / par_merge).
//                  Note: libstdc++'s __gnu_parallel::merge ran serially here (perf showed
//                  __merge_advance with 15 threads spinning), so we split the merge ourselves.
//   v2  v1 + OPT minus BEFORE sorting: the full relation also lives in a hash set, every
//                  candidate is looked up in parallel and the ones already known are dropped
//                  right away. Only the survivors (usually far fewer) are sorted.
//   v3  v2 + OPT lazy merge: the sorted copy of a full relation is only needed when a rule
//                  JOINS with it. New facts are kept as a list of sorted pieces and merged in
//                  one go when full_rel() is called. Relations that are never joined as a whole
//                  (TC's path, SG's sg, taint's Reach, CSPA's ValueAlias - the biggest one) are
//                  then merged once at the very end instead of in every round.
//
// For comparison, Soufflé does a B-tree lookup per candidate, inserts new ones into a
// concurrent B-tree `new`, then merges `new` into the full relation in a SERIAL loop.
#ifndef OMP_BACKEND_H
#define OMP_BACKEND_H

#include <omp.h>
#include <cstring>
#include <parallel/algorithm>
#include "../engine/host_backend.h"

// ---- open-addressing hash set of 64-bit facts (lock-free inserts with CAS) ----
struct HashSet {
    static constexpr uint64_t EMPTY = ~0ULL;    // (0xffffffff, 0xffffffff) is never a real fact here
    std::vector<uint64_t> slot;
    uint64_t mask = 0;
    size_t count = 0;

    bool contains(uint64_t k) const
    {
        if (!mask) return false;
        uint64_t h = mix64(k) & mask;
        for (;;) {
            uint64_t cur = slot[h];
            if (cur == k) return true;
            if (cur == EMPTY) return false;
            h = (h + 1) & mask;
        }
    }
    // returns true if k was not there before (thread safe)
    bool insert(uint64_t k)
    {
        uint64_t h = mix64(k) & mask;
        for (;;) {
            uint64_t cur = __atomic_load_n(&slot[h], __ATOMIC_RELAXED);
            if (cur == k) return false;
            if (cur == EMPTY) {
                if (__atomic_compare_exchange_n(&slot[h], &cur, k, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
                    return true;
                if (cur == k) return false;   // another thread put the same fact here
                continue;                     // another fact took the slot: look again
            }
            h = (h + 1) & mask;
        }
    }
    // make room for n facts in total (load factor <= 0.5): rehash the old slots in parallel
    void reserve(size_t n)
    {
        if (mask && 2 * n <= mask + 1) return;
        size_t cap = 1024;
        while (cap < 2 * n) cap *= 2;
        std::vector<uint64_t> old;
        old.swap(slot);
        slot.assign(cap, EMPTY);
        mask = cap - 1;
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < old.size(); i++)
            if (old[i] != EMPTY) insert(old[i]);
    }
    void insert_all(const HostRel &v)
    {
        reserve(count + v.size());
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < v.size(); i++) insert(v[i]);
        count += v.size();      // callers only insert facts that are not in the set yet
    }
};

struct OmpBackend : HostBackend {
    int version = 1, nthreads = 1;

    struct Full {
        Rel r;                      // sorted, used by the joins
        HashSet hs;                 // v2, v3: the same facts, for fast "already known?" checks
        std::vector<Rel> pending;   // v3: sorted pieces not merged into r yet
    };

    const char *name() { return "omp"; }
    int workers() { return nthreads; }

    void full_init(Full &f, const Rel &r)
    {
        f.r = r;
        if (version >= 2) f.hs.insert_all(r);
    }

    // v3: merge the pending pieces only when somebody needs the sorted relation
    const Rel &full_rel(Full &f)
    {
        if (!f.pending.empty()) {
            double t0 = now_sec();
            Rel all = concat(f.pending);
            f.pending.clear();
            __gnu_parallel::sort(all.begin(), all.end());   // pieces are disjoint: no unique needed
            prof.sort += now_sec() - t0;
            t0 = now_sec();
            merge_into(f, all);
            prof.diff += now_sec() - t0;
        }
        return f.r;
    }

    // ---- v1 OPT: parallel join ----
    Rel join(const Rel &a, const Rel &b, bool flip_out)
    {
        double t0 = now_sec();
        Rel out;
        size_t n = a.size();
        if (n < 2048 || nthreads == 1) {
            join_range(a, 0, n, b, flip_out, out);
            prof.join += now_sec() - t0;
            return out;
        }
        // cut Δ into many pieces, each starting at the beginning of a key group
        int pieces = nthreads * 32;
        std::vector<size_t> cut(pieces + 1);
        for (int p = 0; p <= pieces; p++) {
            size_t c = n * p / pieces;
            while (c > 0 && c < n && col0(a[c - 1]) == col0(a[c])) c++;
            cut[p] = c;
        }
        for (int p = 1; p <= pieces; p++) cut[p] = std::max(cut[p], cut[p - 1]);
        std::vector<Rel> part(pieces);
        #pragma omp parallel for schedule(dynamic, 1)
        for (int p = 0; p < pieces; p++)
            join_range(a, cut[p], cut[p + 1], b, flip_out, part[p]);
        out = concat(part);
        prof.join += now_sec() - t0;
        return out;
    }

    // concatenate per-thread buffers in parallel
    static Rel concat(std::vector<Rel> &part)
    {
        std::vector<size_t> off(part.size() + 1, 0);
        for (size_t p = 0; p < part.size(); p++) off[p + 1] = off[p] + part[p].size();
        Rel out(off.back());
        #pragma omp parallel for schedule(dynamic, 1)
        for (size_t p = 0; p < part.size(); p++) {
            if (!part[p].empty()) memcpy(out.data() + off[p], part[p].data(), part[p].size() * sizeof(tup));
            Rel().swap(part[p]);
        }
        return out;
    }

    void flip(Rel &r)
    {
        double t0 = now_sec();
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < r.size(); i++) r[i] = flip_tup(r[i]);
        prof.sort += now_sec() - t0;
    }

    // ---- v1 OPT: parallel sort + parallel unique ----
    Rel key(Rel &&r)
    {
        double t0 = now_sec();
        __gnu_parallel::sort(r.begin(), r.end());
        parallel_unique(r);
        prof.sort += now_sec() - t0;
        return std::move(r);
    }

    // unique on a sorted array: count survivors per block, prefix sum, compact
    void parallel_unique(Rel &r)
    {
        size_t n = r.size();
        if (n < 100000) { r.erase(std::unique(r.begin(), r.end()), r.end()); return; }
        int B = nthreads * 4;
        std::vector<size_t> cnt(B + 1, 0);
        #pragma omp parallel for schedule(static)
        for (int b = 0; b < B; b++) {
            size_t s = n * b / B, e = n * (b + 1) / B, c = 0;
            for (size_t i = s; i < e; i++) c += (i == 0 || r[i] != r[i - 1]);
            cnt[b + 1] = c;
        }
        for (int b = 0; b < B; b++) cnt[b + 1] += cnt[b];
        Rel out(cnt[B]);
        #pragma omp parallel for schedule(static)
        for (int b = 0; b < B; b++) {
            size_t s = n * b / B, e = n * (b + 1) / B, k = cnt[b];
            for (size_t i = s; i < e; i++)
                if (i == 0 || r[i] != r[i - 1]) out[k++] = r[i];
        }
        r.swap(out);
    }

    // ---- v1 OPT: parallel set difference c \ f (both sorted) ----
    // c is cut into pieces; each piece only needs the part of f between its first and last fact.
    Rel par_difference(const Rel &c, const Rel &f)
    {
        if (c.size() < 100000 || nthreads == 1) {
            Rel d;
            std::set_difference(c.begin(), c.end(), f.begin(), f.end(), std::back_inserter(d));
            return d;
        }
        int P = nthreads * 4;
        std::vector<Rel> part(P);
        #pragma omp parallel for schedule(dynamic, 1)
        for (int p = 0; p < P; p++) {
            size_t s = c.size() * p / P, e = c.size() * (p + 1) / P;
            if (s == e) continue;
            auto fs = std::lower_bound(f.begin(), f.end(), c[s]);
            auto fe = std::upper_bound(fs, f.end(), c[e - 1]);
            std::set_difference(c.begin() + s, c.begin() + e, fs, fe, std::back_inserter(part[p]));
        }
        return concat(part);
    }

    // ---- v1 OPT: parallel merge full.r ∪ delta (both sorted) ----
    // cut the bigger array into pieces; the matching cut in the smaller one is a binary search.
    void merge_into(Full &full, const Rel &delta)
    {
        if (delta.empty()) return;
        const Rel &A = full.r.size() >= delta.size() ? full.r : delta;
        const Rel &B = full.r.size() >= delta.size() ? delta : full.r;
        Rel out(A.size() + B.size());
        if (out.size() < 200000 || nthreads == 1) {
            std::merge(A.begin(), A.end(), B.begin(), B.end(), out.begin());
        } else {
            int P = nthreads * 4;
            std::vector<size_t> ai(P + 1), bi(P + 1);
            for (int p = 0; p <= P; p++) {
                ai[p] = A.size() * p / P;
                // first piece starts at 0 in both arrays (B may have facts smaller than A[0]),
                // last piece ends at the end of both
                if (p == 0) bi[p] = 0;
                else if (p == P) bi[p] = B.size();
                else bi[p] = std::lower_bound(B.begin(), B.end(), A[ai[p]]) - B.begin();
            }
            #pragma omp parallel for schedule(dynamic, 1)
            for (int p = 0; p < P; p++)
                std::merge(A.begin() + ai[p], A.begin() + ai[p + 1], B.begin() + bi[p], B.begin() + bi[p + 1],
                           out.begin() + ai[p] + bi[p]);
        }
        full.r.swap(out);
    }

    Rel diff_merge(Rel &&cand, Full &full)
    {
        if (version >= 2) return diff_merge_hash(std::move(cand), full);
        // ---- v1: sort everything, then parallel set difference + merge ----
        Rel c = key(std::move(cand));
        double t0 = now_sec();
        Rel delta = par_difference(c, full.r);
        merge_into(full, delta);
        prof.diff += now_sec() - t0;
        return delta;
    }

    // ---- v2 OPT: drop known facts with a parallel hash lookup, sort only the survivors ----
    Rel diff_merge_hash(Rel &&cand, Full &full)
    {
        double t0 = now_sec();
        std::vector<Rel> part(nthreads);
        #pragma omp parallel
        {
            Rel &mine = part[omp_get_thread_num()];
            #pragma omp for schedule(static)
            for (size_t i = 0; i < cand.size(); i++)
                if (!full.hs.contains(cand[i])) mine.push_back(cand[i]);
        }
        Rel().swap(cand);
        Rel surv = concat(part);
        prof.diff += now_sec() - t0;

        Rel delta = key(std::move(surv));       // sort + unique the (few) survivors

        t0 = now_sec();
        full.hs.insert_all(delta);
        if (version >= 3) full.pending.push_back(delta);    // v3: merge later, only if needed
        else merge_into(full, delta);
        prof.diff += now_sec() - t0;
        return delta;
    }

    Result result(const std::string &name, Full &f, bool flipped_rel)
    {
        return HostBackend::result(name, full_rel(f), flipped_rel);
    }
    Result result(const std::string &name, const Rel &r, bool flipped_rel)
    {
        return HostBackend::result(name, r, flipped_rel);
    }
};

#endif
