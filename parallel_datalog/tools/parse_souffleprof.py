#!/usr/bin/env python3
"""parse_souffleprof.py - turn Soufflé's profile log (JSON) into a phase breakdown (our code).

usage: parse_souffleprof.py prof.log [--iters iterations.csv]

Soufflé's profiler records, for every relation and every fixpoint iteration:
  - runtime of each recursive rule = JOIN + DEDUP CHECK (Reach.contains) + INSERT into `new`
    (the three are fused in one loop in the generated C++)
  - copytime = MERGE of `new` into the full relation (a SERIAL loop in Soufflé's code)
  - num-tuples = new facts found in that iteration (the Δ size)
plus load time per input relation and the non-recursive rules.
We print one summary line per relation and (optionally) one CSV row per iteration.
Times in the log are microseconds.
"""
import argparse
import csv
import json


def span(d):
    return (d["end"] - d["start"]) / 1e6 if d else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--iters")
    a = ap.parse_args()
    with open(a.log) as f:
        prog = json.load(f)["root"]["program"]

    rows = []
    total_rule = total_copy = total_load = total_nonrec = 0.0
    print(f"{'relation':14} {'iters':>6} {'rules_s':>9} {'copy_s':>9} {'nonrec_s':>9} {'load_s':>8} {'tuples':>12}  max_delta")
    for name, rel in prog.get("relation", {}).items():
        load = span(rel.get("loadtime"))
        nonrec = sum(span(r["runtime"]) for r in rel.get("non-recursive-rule", {}).values()) \
            if isinstance(rel.get("non-recursive-rule"), dict) else 0.0
        it = rel.get("iteration", {})
        rule_t = copy_t = 0.0
        maxd = 0
        for k in sorted(it, key=int):
            x = it[k]
            r = 0.0
            for rule in x.get("recursive-rule", {}).values():
                for v in rule.values():
                    r += span(v.get("runtime"))
            c = span(x.get("copytime"))
            n = x.get("num-tuples", 0)
            rule_t += r
            copy_t += c
            maxd = max(maxd, n)
            rows.append({"relation": name, "iteration": int(k), "new_tuples": n,
                         "rules_s": round(r, 6), "copy_s": round(c, 6)})
        total_rule += rule_t
        total_copy += copy_t
        total_load += load
        total_nonrec += nonrec
        print(f"{name:14} {len(it):6d} {rule_t:9.3f} {copy_t:9.3f} {nonrec:9.3f} {load:8.3f} "
              f"{rel.get('num-tuples', 0):12d}  {maxd}")
    tot = total_rule + total_copy + total_nonrec + total_load
    if tot > 0:
        print(f"{'TOTAL':14} {'':6} {total_rule:9.3f} {total_copy:9.3f} {total_nonrec:9.3f} {total_load:8.3f}")
        print(f"share: recursive rules (join+dedup+insert) {100 * total_rule / tot:.1f}%, "
              f"merge new->full (copy) {100 * total_copy / tot:.1f}%, "
              f"non-recursive rules {100 * total_nonrec / tot:.1f}%, loading {100 * total_load / tot:.1f}%")
    if a.iters and rows:
        with open(a.iters, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)


if __name__ == "__main__":
    main()
