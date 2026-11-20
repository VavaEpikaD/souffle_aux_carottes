#!/usr/bin/env python3
"""make_cases.py - tiny test cases with hand-checked expected output (our code).

usage: make_cases.py OUTDIR
Writes OUTDIR/<case>/rules (name of the rule set), facts/*.facts and, where the result was
checked by hand, expected/<Relation>.csv. Cases without expected/ use Souffle as the oracle.
"""
import os
import random
import sys

CASES = {}

# ---- taint: the example from the project plan -------------------------------------
#   void handle(int sock) { char hdr[8]; recv(sock, hdr, 8, 0);   // 100 = recv@3, 1 = hdr
#       int n = parse_len(hdr);                                   // 2 = n
#       process(n);                                               // 3 = process.n
#       int m = check_len(n);  /* sanitizer */                    // 5 = m (sanitized)
#       int k = m;                                                // 6 = k
#       memcpy(buf2, x, k); }                                     // 201 = memcpy@k
#   void process(int n) { memcpy(buf, global_data, n); }          // 200 = memcpy@9
#   int g = global_len; memcpy(buf3, y, g);                       // 4 = global_len, 7 = g, 202
CASES["taint_example"] = ("taint", {
    "Source": [(100, 1)],
    "Flow": [(1, 2), (2, 3), (2, 5), (5, 6), (4, 7)],
    "Sink": [(200, 3), (201, 6), (202, 7)],
    "Sanitized": [(5,)],
}, {
    "Reach": [(100, 1), (100, 2), (100, 3)],
    "Alert": [(100, 200)],     # only the unchecked memcpy is reported
})

# ---- TC: a cycle 1->2->3->1, an exit 3->4, a self loop on 5 --------------------------
CASES["tc_cycle"] = ("tc", {
    "edge": [(1, 2), (2, 3), (3, 1), (3, 4), (5, 5)],
}, {
    "path": [(a, b) for a in (1, 2, 3) for b in (1, 2, 3, 4)] + [(5, 5)],
})

# ---- SG: tree 1 -> {2,3}, 2 -> {4,5}, 3 -> {6} ------------------------------------------
CASES["sg_tree"] = ("sg", {
    "edge": [(1, 2), (1, 3), (2, 4), (2, 5), (3, 6)],
}, {
    "sg": [(2, 3), (3, 2), (4, 5), (5, 4), (4, 6), (5, 6), (6, 4), (6, 5)],
})


def random_cases():
    """small random inputs: no expected output, Souffle is the oracle"""
    rng = random.Random(5)
    out = {}
    # CSPA: 40 variables, random assign / dereference
    n = 40
    out["cspa_random"] = ("cspa", {
        "assign": sorted({(rng.randrange(n), rng.randrange(n)) for _ in range(60)}),
        "dereference": sorted({(rng.randrange(n), rng.randrange(n)) for _ in range(40)}),
    }, None)
    # TC / SG on a random graph with hubs
    edges = sorted({(rng.randrange(300) if rng.random() < .8 else rng.randrange(5), rng.randrange(300))
                    for _ in range(700)})
    out["tc_random"] = ("tc", {"edge": edges}, None)
    out["sg_random"] = ("sg", {"edge": edges[:400]}, None)
    # taint on the same graph: 8 sources, some sinks, some sanitized
    out["taint_random"] = ("taint", {
        "Source": [(1000 + i, rng.randrange(300)) for i in range(8)],
        "Flow": edges,
        "Sink": [(2000 + i, rng.randrange(300)) for i in range(20)],
        "Sanitized": sorted({(rng.randrange(300),) for _ in range(15)}),
    }, None)
    return out


def write(outdir, name, rules, facts, expected):
    d = os.path.join(outdir, name)
    os.makedirs(os.path.join(d, "facts"), exist_ok=True)
    with open(os.path.join(d, "rules"), "w") as f:
        f.write(rules + "\n")
    for rel, rows in facts.items():
        with open(os.path.join(d, "facts", rel + ".facts"), "w") as f:
            for r in rows:
                f.write("\t".join(map(str, r)) + "\n")
    if expected:
        os.makedirs(os.path.join(d, "expected"), exist_ok=True)
        for rel, rows in expected.items():
            with open(os.path.join(d, "expected", rel + ".csv"), "w") as f:
                for r in sorted(set(rows)):
                    f.write("\t".join(map(str, r)) + "\n")


def main():
    outdir = sys.argv[1]
    all_cases = dict(CASES)
    all_cases.update(random_cases())
    for name, (rules, facts, expected) in all_cases.items():
        write(outdir, name, rules, facts, expected)


if __name__ == "__main__":
    main()
