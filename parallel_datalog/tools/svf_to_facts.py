#!/usr/bin/env python3
"""svf_to_facts.py - facts for our rules from SVF's pointer-assignment graph (our code).

usage: svf_to_facts.py pag.txt OUTDIR_CSPA OUTDIR_FLOW

Input: the output of `wpa -ander -print-pag prog.bc` (SVF, an existing LLVM-based analyzer),
one edge per line:  "<src> -- <Kind> --> <dst>"  with Kind in Addr, Copy, Load, Store, Gep,
VariantGep, Call, Ret, ... (numbers are SVF node ids).

We map them the way Graspan's front end encodes C programs for CSPA:
  q = p   (Copy, Gep, VariantGep, Call: actual->formal, Ret: return->call site)
          -> assign(p, q)                 "the value of p flows into q"
  q = *p  (Load p -> q)    -> dereference(p, *p) and assign(*p, q)
  *q = p  (Store p -> q)   -> dereference(q, *q) and assign(p, *q)
  p = &o  (Addr o -> p)    -> dereference(p, o)
where *p is one extra node per pointer p (id = p + OFFSET). Fields are not distinguished
(field-insensitive), as in the project scope.

The same value-flow edges (every assign) are written to flow_edges.tsv, which
make_taint_facts.py turns into the taint inputs (Flow + Source/Sink/Sanitized).
"""
import os
import sys

COPY_LIKE = {"Copy", "Gep", "VariantGep", "NormalGep", "Call", "Ret", "ThreadFork", "ThreadJoin"}


def main():
    pag, out_cspa, out_flow = sys.argv[1:4]
    edges = []
    maxid = 0
    with open(pag) as f:
        for line in f:
            p = line.split()
            # "<src> -- <Kind> --> <dst>"
            if len(p) == 5 and p[1] == "--" and p[3] == "-->" and p[0].isdigit() and p[4].isdigit():
                s, kind, d = int(p[0]), p[2], int(p[4])
                edges.append((s, kind, d))
                maxid = max(maxid, s, d)
    off = maxid + 1
    assign, deref = set(), set()
    for s, kind, d in edges:
        if kind in COPY_LIKE:
            assign.add((s, d))
        elif kind == "Load":            # d = *s
            deref.add((s, s + off))
            assign.add((s + off, d))
        elif kind == "Store":           # *d = s
            deref.add((d, d + off))
            assign.add((s, d + off))
        elif kind == "Addr":            # d = &s
            deref.add((d, s))
    for path, rows in ((os.path.join(out_cspa, "assign.facts"), assign),
                       (os.path.join(out_cspa, "dereference.facts"), deref),
                       (os.path.join(out_flow, "flow_edges.tsv"), assign)):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            for a, b in sorted(rows):
                f.write(f"{a}\t{b}\n")
    kinds = {}
    for _, k, _ in edges:
        kinds[k] = kinds.get(k, 0) + 1
    print(f"{pag}: {len(edges)} SVF edges {kinds} -> assign {len(assign)}, dereference {len(deref)}")


if __name__ == "__main__":
    main()
