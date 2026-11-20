// thrust_backend.h - Person C: GPU back-end of the 6 operators, written with Thrust (our code)
//
// Only Thrust algorithms + small functors, no hand-written kernels yet. That way the SAME
// code compiles with
//   - nvcc  -> runs on an NVIDIA GPU (Google Colab), relations live in GPU memory
//   - g++ with THRUST_DEVICE_SYSTEM=OMP -> runs on the CPU here (correctness tests, CPU data point)
//
// A relation is a device_vector<tup> kept sorted (all data stays on the device between rounds).
//   v1 OPT join without atomics: for every Δ fact, binary search its key range in the other
//          relation (vectorized lower_bound / upper_bound), exclusive scan of the range sizes
//          gives every fact its own output slot, then one parallel "expand" writes the pairs.
//      dedup / minus / merge: sort + unique, set_difference, merge (all parallel on the device).
// Next steps (not in v1): hash-indexed sorted arrays like GPUlog, warp per hub key,
// running tiny rounds on the CPU.
#ifndef THRUST_BACKEND_H
#define THRUST_BACKEND_H

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <thrust/set_operations.h>
#include <thrust/merge.h>
#include <thrust/binary_search.h>
#include <thrust/scan.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <thrust/remove.h>
#include <thrust/for_each.h>
#include <thrust/iterator/counting_iterator.h>
#include "../engine/io.h"
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif

// ---- functors (run on the device) ----
struct key_lo_f { HD tup operator()(tup t) const { return make_tup(col0(t), 0); } };
struct key_hi_f { HD tup operator()(tup t) const { return make_tup(col0(t), 0xffffffffu); } };
struct flip_f { HD tup operator()(tup t) const { return flip_tup(t); } };
struct eq_cols_f { HD bool operator()(tup t) const { return col0(t) == col1(t); } };
struct hash_f {
    bool flipped;
    HD uint64_t operator()(tup t) const { return mix64(flipped ? flip_tup(t) : t); }
};
struct in_unary_f {
    const uint8_t *u;
    size_t n;
    HD bool operator()(tup t) const { return col0(t) < n && u[col0(t)]; }
};
struct minus_f { HD size_t operator()(size_t hi, size_t lo) const { return hi - lo; } };
// writes all pairs of Δ fact i: out[off[i] + j] for every b fact in [lo[i], hi[i])
struct expand_f {
    const tup *a, *b;
    const size_t *lo, *hi, *off;
    tup *out;
    bool flip_out;
    HD void operator()(size_t i) const
    {
        size_t o = off[i];
        uint32_t x = col1(a[i]);
        for (size_t j = lo[i]; j < hi[i]; j++)
            out[o++] = flip_out ? make_tup(col1(b[j]), x) : make_tup(x, col1(b[j]));
    }
};

struct ThrustBackend {
    typedef thrust::device_vector<tup> Rel;
    struct Full { Rel r; };
    typedef thrust::device_vector<uint8_t> Unary;

    Prof prof;
    std::string outdir;
    int nthreads = 1;

#ifdef __CUDACC__
    const char *name() { return "thrust_gpu"; }
    int workers() { return 1; }
    static void sync() { cudaDeviceSynchronize(); }
#else
    const char *name() { return "thrust_omp"; }
    int workers() { return nthreads; }
    static void sync() {}
#endif
    bool is_root() { return true; }
    void barrier() { sync(); }

    // host -> device (counted as transfer time), then sorted
    Rel load(const std::vector<tup> &host)
    {
        double t0 = now_sec();
        Rel r(host.begin(), host.end());
        sync();
        prof.comm += now_sec() - t0;
        return key(std::move(r));
    }
    Unary make_unary(const std::vector<uint32_t> &v)
    {
        uint32_t mx = 0;
        for (uint32_t x : v) mx = std::max(mx, x);
        std::vector<uint8_t> h(v.empty() ? 0 : (size_t)mx + 1, 0);
        for (uint32_t x : v) h[x] = 1;
        return Unary(h.begin(), h.end());
    }
    void full_init(Full &f, const Rel &r) { f.r = r; }
    const Rel &full_rel(const Full &f) { return f.r; }
    void append(Rel &a, Rel &&b) { a.insert(a.end(), b.begin(), b.end()); }
    Rel copy(const Rel &r) { return r; }
    uint64_t size(const Rel &r) { return r.size(); }

    // ---- join: binary search + scan + expand (no atomics) ----
    Rel join(const Rel &a, const Rel &b, bool flip_out)
    {
        double t0 = now_sec();
        size_t n = a.size();
        if (n == 0 || b.empty()) return Rel();
        thrust::device_vector<tup> q(n);
        thrust::device_vector<size_t> lo(n), hi(n), cnt(n), off(n);
        thrust::transform(a.begin(), a.end(), q.begin(), key_lo_f());
        thrust::lower_bound(b.begin(), b.end(), q.begin(), q.end(), lo.begin());
        thrust::transform(a.begin(), a.end(), q.begin(), key_hi_f());
        thrust::upper_bound(b.begin(), b.end(), q.begin(), q.end(), hi.begin());
        thrust::transform(hi.begin(), hi.end(), lo.begin(), cnt.begin(), minus_f());
        thrust::exclusive_scan(cnt.begin(), cnt.end(), off.begin());
        size_t total = (size_t)off[n - 1] + (size_t)cnt[n - 1];
        Rel out(total);
        expand_f f;
        f.a = thrust::raw_pointer_cast(a.data());
        f.b = thrust::raw_pointer_cast(b.data());
        f.lo = thrust::raw_pointer_cast(lo.data());
        f.hi = thrust::raw_pointer_cast(hi.data());
        f.off = thrust::raw_pointer_cast(off.data());
        f.out = thrust::raw_pointer_cast(out.data());
        f.flip_out = flip_out;
        thrust::for_each(thrust::counting_iterator<size_t>(0), thrust::counting_iterator<size_t>(n), f);
        sync();
        prof.join += now_sec() - t0;
        return out;
    }

    void flip(Rel &r) { thrust::transform(r.begin(), r.end(), r.begin(), flip_f()); }
    void filter_neq(Rel &r) { r.erase(thrust::remove_if(r.begin(), r.end(), eq_cols_f()), r.end()); }
    void filter_not(Rel &r, const Unary &u)
    {
        in_unary_f f;
        f.u = thrust::raw_pointer_cast(u.data());
        f.n = u.size();
        r.erase(thrust::remove_if(r.begin(), r.end(), f), r.end());
    }

    Rel key(Rel &&r)
    {
        double t0 = now_sec();
        thrust::sort(r.begin(), r.end());
        r.erase(thrust::unique(r.begin(), r.end()), r.end());
        sync();
        prof.sort += now_sec() - t0;
        return std::move(r);
    }

    Rel diff_merge(Rel &&cand, Full &full)
    {
        Rel c = key(std::move(cand));
        double t0 = now_sec();
        Rel delta(c.size());
        auto end = thrust::set_difference(c.begin(), c.end(), full.r.begin(), full.r.end(), delta.begin());
        delta.resize(end - delta.begin());
        Rel().swap(c);
        if (!delta.empty()) {
            Rel merged(full.r.size() + delta.size());
            thrust::merge(full.r.begin(), full.r.end(), delta.begin(), delta.end(), merged.begin());
            full.r.swap(merged);
        }
        sync();
        prof.diff += now_sec() - t0;
        return delta;
    }

    Result result(const std::string &name, const Rel &r, bool flipped_rel)
    {
        Result res;
        res.name = name;
        res.size = r.size();
        hash_f h;
        h.flipped = flipped_rel;
        res.hash = thrust::transform_reduce(r.begin(), r.end(), h, (uint64_t)0, thrust::plus<uint64_t>());
        if (!outdir.empty()) {
            double t0 = now_sec();
            thrust::host_vector<tup> hv(r);
            std::vector<tup> v(hv.begin(), hv.end());
            prof.comm += now_sec() - t0;
            write_csv(outdir, name, flipped_rel ? flipped(v) : v);
        }
        return res;
    }
};

#endif
