// host_backend.h - SEQUENTIAL back-end: the reference implementation of the 6 operators (our code)
//
// A relation is a std::vector<tup>. "Keyed" relations are sorted and unique.
// The OpenMP back-end (omp/) derives from this class and replaces the slow operators.
#ifndef HOST_BACKEND_H
#define HOST_BACKEND_H

#include <algorithm>
#include "io.h"

struct HostBackend {
    typedef HostRel Rel;    // std::vector<tup> without zero-filling (see rel.h)
    struct Full { Rel r; };                 // the full relation, sorted
    typedef std::vector<uint8_t> Unary;     // bitmap: U[v] = 1 if v is in the relation

    Prof prof;
    std::string outdir;

    const char *name() { return "seq"; }
    int workers() { return 1; }
    bool is_root() { return true; }
    void barrier() {}

    // ---- helpers ----
    Rel load(const std::vector<tup> &host)
    {
        Rel r(host.begin(), host.end());
        return key(std::move(r));
    }
    Unary make_unary(const std::vector<uint32_t> &v)
    {
        uint32_t mx = 0;
        for (uint32_t x : v) mx = std::max(mx, x);
        Unary u(v.empty() ? 0 : (size_t)mx + 1, 0);
        for (uint32_t x : v) u[x] = 1;
        return u;
    }
    void full_init(Full &f, const Rel &r) { f.r = r; }
    const Rel &full_rel(const Full &f) { return f.r; }
    void append(Rel &a, Rel &&b) { a.insert(a.end(), b.begin(), b.end()); }
    Rel copy(const Rel &r) { return r; }
    uint64_t size(const Rel &r) { return r.size(); }

    // ---- operator 1: join ----
    // a(k,x) and b(k,y) are sorted, so the facts with the same key k are next to each other.
    // For every key group of a we find the same key in b (binary search from where we are)
    // and output every pair.
    Rel join(const Rel &a, const Rel &b, bool flip_out)
    {
        double t0 = now_sec();
        Rel out;
        join_range(a, 0, a.size(), b, flip_out, out);
        prof.join += now_sec() - t0;
        return out;
    }

    // join the facts a[i0 .. i1) (i0 must be the start of a key group); used by OpenMP too
    static void join_range(const Rel &a, size_t i0, size_t i1, const Rel &b, bool flip_out, Rel &out)
    {
        size_t i = i0;
        const tup *bb = b.data(), *be = b.data() + b.size();
        const tup *pos = bb;
        while (i < i1) {
            uint32_t k = col0(a[i]);
            size_t j = i;
            while (j < a.size() && col0(a[j]) == k) j++;        // a[i..j) = key group
            pos = std::lower_bound(pos, be, make_tup(k, 0));
            const tup *q = pos;
            while (q < be && col0(*q) == k) q++;               // b[pos..q) = same key
            for (size_t x = i; x < j; x++)
                for (const tup *y = pos; y < q; y++)
                    out.push_back(flip_out ? make_tup(col1(*y), col1(a[x])) : make_tup(col1(a[x]), col1(*y)));
            i = j;
        }
    }

    // ---- operators 2, 3: flip, filters ----
    void flip(Rel &r)
    {
        double t0 = now_sec();
        for (auto &t : r) t = flip_tup(t);
        prof.sort += now_sec() - t0;
    }
    void filter_neq(Rel &r)
    {
        r.erase(std::remove_if(r.begin(), r.end(), [](tup t) { return col0(t) == col1(t); }), r.end());
    }
    void filter_not(Rel &r, const Unary &u)
    {
        r.erase(std::remove_if(r.begin(), r.end(),
                               [&](tup t) { return col0(t) < u.size() && u[col0(t)]; }), r.end());
    }

    // ---- operator 4: key = sort + dedup ----
    Rel key(Rel &&r)
    {
        double t0 = now_sec();
        std::sort(r.begin(), r.end());
        r.erase(std::unique(r.begin(), r.end()), r.end());
        prof.sort += now_sec() - t0;
        return std::move(r);
    }

    // ---- operators 5, 6: minus + merge ----
    Rel diff_merge(Rel &&cand, Full &full)
    {
        Rel c = key(std::move(cand));
        double t0 = now_sec();
        Rel delta;
        std::set_difference(c.begin(), c.end(), full.r.begin(), full.r.end(), std::back_inserter(delta));
        Rel merged(full.r.size() + delta.size());
        std::merge(full.r.begin(), full.r.end(), delta.begin(), delta.end(), merged.begin());
        full.r.swap(merged);
        prof.diff += now_sec() - t0;
        return delta;
    }

    // ---- result: size + hash, CSV if -D was given ----
    Result result(const std::string &name, const Rel &r, bool flipped_rel)
    {
        Result res;
        res.name = name;
        res.size = r.size();
        for (tup t : r) res.hash += mix64(flipped_rel ? flip_tup(t) : t);
        // STA_ONLY=<Relation> writes just that relation (e.g. ValueFlow, without the huge ValueAlias)
        const char *only = getenv("STA_ONLY");
        if (!outdir.empty() && (!only || name == only)) {
            std::vector<tup> v(r.begin(), r.end());
            write_csv(outdir, name, flipped_rel ? flipped(v) : v);
        }
        return res;
    }
};

#endif
