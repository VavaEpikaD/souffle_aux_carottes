#!/usr/bin/env python3
"""make_taint_facts.py - taint inputs from a flow graph (our code).

usage: make_taint_facts.py EDGES OUTDIR [--sources 64] [--sinks 0.01] [--sanitized 0.01] [--seed 1]

EDGES is a tab-separated "from<TAB>to" file. We use real program data flow when we have it
(Graspan's assign(src, dest) = the value of src flows into dest, for httpd / linux / postgresql)
and benchmark graphs otherwise. Our graphs have no function names, so, as the project plan
says (Section 4), sources, sinks and sanitizers are chosen at random with a fixed seed:
  Source(s, v)  s = 0..S-1, v = a random variable that has outgoing flow
  Sink(k, v)    k = 1000000 + i, for a fraction of the variables
  Sanitized(v)  a fraction of the variables
"""
import argparse
import os
import random


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("edges")
    ap.add_argument("outdir")
    ap.add_argument("--sources", type=int, default=64)
    ap.add_argument("--sinks", type=float, default=0.01)
    ap.add_argument("--sanitized", type=float, default=0.01)
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args()

    rng = random.Random(a.seed)
    os.makedirs(a.outdir, exist_ok=True)
    nodes, starts = set(), set()
    with open(a.edges) as f, open(os.path.join(a.outdir, "Flow.facts"), "w") as out:
        for line in f:
            p = line.split()
            if len(p) < 2:
                continue
            out.write(p[0] + "\t" + p[1] + "\n")
            nodes.add(int(p[0]))
            nodes.add(int(p[1]))
            starts.add(int(p[0]))
    nodes = sorted(nodes)
    starts = sorted(starts)

    with open(os.path.join(a.outdir, "Source.facts"), "w") as f:
        for s, v in enumerate(rng.sample(starts, min(a.sources, len(starts)))):
            f.write(f"{s}\t{v}\n")
    with open(os.path.join(a.outdir, "Sink.facts"), "w") as f:
        for i, v in enumerate(rng.sample(nodes, int(len(nodes) * a.sinks))):
            f.write(f"{1000000 + i}\t{v}\n")
    with open(os.path.join(a.outdir, "Sanitized.facts"), "w") as f:
        for v in sorted(rng.sample(nodes, int(len(nodes) * a.sanitized))):
            f.write(f"{v}\n")
    print(f"{a.outdir}: {len(nodes)} variables, {len(starts)} with outgoing flow")


if __name__ == "__main__":
    main()
