// programs.h - the rule sets, written as calls to the 6 operators (our code)
//
// Every back-end B (seq, omp, mpi, thrust) provides the same operators:
//
//   Rel   B::load(host facts)          facts -> relation keyed by column 0 (sorted, unique; MPI: own part)
//   Rel   B::join(a, b, flip)          a(k,x), b(k,y), both keyed by k  ->  (x,y)  or (y,x) if flip.
//                                      Output is NOT sorted and may contain duplicates.
//   void  B::flip(r)                   (x,y) -> (y,x)
//   void  B::filter_neq(r)             keep x != y
//   void  B::filter_not(r, U)          drop facts whose column 0 is in the unary relation U
//   Rel   B::key(r)                    sort + unique (MPI: first send each fact to the owner of col 0)
//   Rel   B::diff_merge(cand, full)    Δ = key(cand) \ full ;  full = full ∪ Δ ;  return Δ
//
// plus helpers: full_init, full_rel, append, size (global), result.
//
// SEMI-NAIVE EVALUATION: every round only the facts that are new since the last round (Δ)
// are joined. A rule with two recursive atoms R1, R2 is evaluated as  Δ1⋈R2  ∪  R1⋈Δ2,
// where R1, R2 already contain Δ (diff_merge merges right away). A rule with three atoms is
// done as two joins with a temporary relation in between.
//
// To join on column 1 of a relation we keep a flipped copy of it (keyed by that column).
#ifndef PROGRAMS_H
#define PROGRAMS_H

#include "io.h"

// ---------------------------------------------------------------------------------------
// TC:  path(x,y) :- edge(x,y).   path(x,y) :- path(x,z), edge(z,y).
// path is kept flipped, PF(y,x), keyed by its end node, so the join key z is column 0.
template <class B>
void prog_tc(B &be, Facts &F, std::vector<Result> &out)
{
    auto E = be.load(F.binary("edge"));                 // E(z,y)
    auto dPF = be.load(flipped(F.binary("edge")));      // base: path = edge  ->  PF(y,x)
    typename B::Full PF;
    be.full_init(PF, dPF);
    while (be.size(dPF) > 0) {
        auto cand = be.join(dPF, E, true);              // ΔPF(z,x) ⋈ E(z,y) -> (y,x)
        dPF = be.diff_merge(std::move(cand), PF);
        be.prof.iters++;
    }
    out.push_back(be.result("path", be.full_rel(PF), true));
}

// ---------------------------------------------------------------------------------------
// SG:  sg(x,y) :- edge(p,x), edge(p,y), x != y.
//      sg(x,y) :- edge(a,x), sg(a,b), edge(b,y).
template <class B>
void prog_sg(B &be, Facts &F, std::vector<Result> &out)
{
    auto E = be.load(F.binary("edge"));                 // E(p,x) keyed by parent
    auto base = be.join(E, E, false);                   // (x,y) siblings
    be.filter_neq(base);
    auto dSG = be.key(std::move(base));
    typename B::Full SG;
    be.full_init(SG, dSG);
    while (be.size(dSG) > 0) {
        auto T = be.key(be.join(dSG, E, false));        // ΔSG(a,b) ⋈ E(a,x) -> T(b,x)
        auto cand = be.join(T, E, false);               // T(b,x) ⋈ E(b,y)   -> (x,y)
        dSG = be.diff_merge(std::move(cand), SG);
        be.prof.iters++;
    }
    out.push_back(be.result("sg", be.full_rel(SG), false));
}

// ---------------------------------------------------------------------------------------
// TAINT:  Reach(s,v) :- Source(s,v).
//         Reach(s,w) :- Reach(s,v), Flow(v,w), !Sanitized(w).
//         Alert(s,k) :- Reach(s,v), Sink(k,v).
// Reach is kept flipped, RV(v,s), keyed by the variable (the join column).
template <class B>
void prog_taint(B &be, Facts &F, std::vector<Result> &out)
{
    auto Flow = be.load(F.binary("Flow"));              // Flow(v,w)
    auto San = be.make_unary(F.unary("Sanitized"));
    auto dRV = be.load(flipped(F.binary("Source")));    // RV(v,s)
    typename B::Full RV;
    be.full_init(RV, dRV);
    while (be.size(dRV) > 0) {
        auto cand = be.join(dRV, Flow, true);           // ΔRV(v,s) ⋈ Flow(v,w) -> (w,s)
        be.filter_not(cand, San);                       // !Sanitized(w)
        dRV = be.diff_merge(std::move(cand), RV);
        be.prof.iters++;
    }
    auto SinkV = be.load(flipped(F.binary("Sink")));    // Sink(k,v) -> (v,k)
    auto alert = be.key(be.join(be.full_rel(RV), SinkV, false));   // (s,k)
    out.push_back(be.result("Reach", be.full_rel(RV), true));
    out.push_back(be.result("Alert", alert, false));
}

// ---------------------------------------------------------------------------------------
// CSPA (Graspan, rules as in GPUlog's cspa.dl):
//   VF(y,x) :- assign(y,x).          VF(x,x) :- assign(x,_).     VF(x,x) :- assign(_,x).
//   MA(x,x) :- assign(_,x).          MA(x,x) :- assign(x,_).
//   VF(x,y) :- VF(x,z), VF(z,y).
//   VF(x,y) :- assign(x,z), MA(z,y).
//   VA(x,y) :- VF(z,x), VF(z,y).
//   MA(x,w) :- dereference(y,x), VA(y,z), dereference(z,w).
//   VA(x,y) :- VF(z,x), MA(z,w), VF(w,y).
// The last rule has three recursive atoms; we split it with an auxiliary relation
//   VFMA(w,x) :- VF(z,x), MA(z,w).      VA(x,y) :- VFMA(w,x), VF(w,y).
// which gives the same VA (VFMA is our own helper, not in the output).
template <class B>
void prog_cspa(B &be, Facts &F, std::vector<Result> &out)
{
    const auto &assign = F.binary("assign");
    std::vector<tup> vf0(assign), ma0;
    for (tup t : assign) {
        vf0.push_back(make_tup(col0(t), col0(t)));
        vf0.push_back(make_tup(col1(t), col1(t)));
        ma0.push_back(make_tup(col0(t), col0(t)));
        ma0.push_back(make_tup(col1(t), col1(t)));
    }
    auto Ar = be.load(flipped(assign));                  // assign(x,z) keyed by z: (z,x)
    auto D = be.load(F.binary("dereference"));           // D(y,x) keyed by y

    auto dVF = be.load(vf0);                             // VF(x,y)  keyed by x
    auto dVFr = be.load(flipped(vf0));                   // VF flipped: (y,x) keyed by y
    auto dMA = be.load(ma0);                             // MA(x,y)  keyed by x
    auto dVA = be.load(std::vector<tup>());              // VA(x,y)  keyed by x
    auto dVFMA = be.load(std::vector<tup>());            // VFMA(w,x) keyed by w
    typename B::Full VF, VFr, MA, VA, VFMA;
    be.full_init(VF, dVF);
    be.full_init(VFr, dVFr);
    be.full_init(MA, dMA);
    be.full_init(VA, dVA);
    be.full_init(VFMA, dVFMA);

    while (be.size(dVF) + be.size(dMA) + be.size(dVA) + be.size(dVFMA) > 0) {
        // VF(x,y) :- VF(x,z), VF(z,y)      join on z: VFr(z,x) with VF(z,y)
        auto cVF = be.join(dVFr, be.full_rel(VF), false);
        be.append(cVF, be.join(be.full_rel(VFr), dVF, false));
        // VF(x,y) :- assign(x,z), MA(z,y)   ΔMA(z,y) ⋈ Ar(z,x) -> (x,y)
        be.append(cVF, be.join(dMA, Ar, true));

        // VA(x,y) :- VF(z,x), VF(z,y)
        auto cVA = be.join(dVF, be.full_rel(VF), false);
        be.append(cVA, be.join(be.full_rel(VF), dVF, false));
        // VA(x,y) :- VFMA(w,x), VF(w,y)
        be.append(cVA, be.join(dVFMA, be.full_rel(VF), false));
        be.append(cVA, be.join(be.full_rel(VFMA), dVF, false));

        // VFMA(w,x) :- VF(z,x), MA(z,w)     -> (w,x)
        auto cVFMA = be.join(dVF, be.full_rel(MA), true);
        be.append(cVFMA, be.join(be.full_rel(VF), dMA, true));

        // MA(x,w) :- D(y,x), VA(y,z), D(z,w)
        auto T = be.key(be.join(dVA, D, false));         // ΔVA(y,z) ⋈ D(y,x) -> T(z,x)
        auto cMA = be.join(T, D, false);                  // T(z,x) ⋈ D(z,w)   -> (x,w)

        // new deltas (every full relation already contains the old deltas)
        dVF = be.diff_merge(std::move(cVF), VF);
        auto fl = be.copy(dVF);
        be.flip(fl);
        dVFr = be.diff_merge(std::move(fl), VFr);
        dVA = be.diff_merge(std::move(cVA), VA);
        dMA = be.diff_merge(std::move(cMA), MA);
        dVFMA = be.diff_merge(std::move(cVFMA), VFMA);
        be.prof.iters++;
    }
    out.push_back(be.result("ValueFlow", be.full_rel(VF), false));
    out.push_back(be.result("ValueAlias", be.full_rel(VA), false));
    out.push_back(be.result("MemoryAlias", be.full_rel(MA), false));
}

#endif
