#!/usr/bin/env python3
"""plot.py - tables and charts from the profile and benchmark results (our code).

usage: python3 scripts/plot.py        -> results/summary.md, results/*.png

Medians over the repetitions. Speedups are computed on WALL time of the whole process
(reading the facts + solving), the same measure for Soufflé and our engines.
"""
import csv
import glob
import os
import re
import statistics
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
INK, INK2, GRID, BG = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
COL = {"souffle": "#52514e", "omp3": "#2a78d6", "mpi": "#eb6834", "thrust": "#1baf7a"}
plt.rcParams.update({"font.size": 10, "axes.edgecolor": INK2, "axes.labelcolor": INK, "text.color": INK,
                     "xtick.color": INK2, "ytick.color": INK2, "axes.grid": True, "grid.color": GRID,
                     "axes.spines.top": False, "axes.spines.right": False, "figure.facecolor": BG,
                     "axes.facecolor": BG, "savefig.dpi": 130})

ENGINE_NAME = {"souffle_seq": "Soufflé (serial code)", "souffle_par": "Soufflé (parallel code)",
               "seq": "ours seq", "omp1": "omp v1", "omp2": "omp v2", "omp3": "omp v3",
               "mpi": "mpi v1", "thrust": "thrust (CPU)"}


def load_bench():
    rows = defaultdict(list)
    status = {}
    path = os.path.join(RES, "bench", "bench.csv")
    if not os.path.exists(path):
        return {}, {}
    for r in csv.DictReader(open(path)):
        key = (r["input"], r["engine"], int(r["threads"]))
        if r["status"] == "ok":
            rows[key].append(r)
        else:
            status[key] = r["status"]
    med = {}
    for k, v in rows.items():
        med[k] = {
            "wall": statistics.median(float(x["wall_s"]) for x in v),
            "solve": statistics.median(float(x["solve_s"]) for x in v if x["solve_s"]) if v[0]["solve_s"] else None,
            "rss": statistics.median(float(x["maxrss_MB"]) for x in v),
            "split": [statistics.median(float(x[c]) for x in v) if v[0][c] else 0.0
                      for c in ("join_s", "sort_s", "diff_s", "comm_s")],
            "iters": v[0]["iters"], "sizes": v[0]["sizes"],
        }
    return med, status


def fmt(x):
    return "-" if x is None else (f"{x:.1f}" if x >= 10 else f"{x:.2f}")


def cell(med, status, key):
    if key in med:
        return fmt(med[key]["wall"])
    return status.get(key, "-")


def main():
    md = []
    # ---------------- profile ----------------
    prof_files = sorted(glob.glob(os.path.join(RES, "profile", "*_phases_j*.txt")))
    if prof_files:
        md.append("### Soufflé profile: where the time goes\n")
        md.append("| input | threads | recursive rules (join + dedup check + insert into new) | merge new → full (serial copy) | other (non-recursive rules, loading) |")
        md.append("|---|---|---|---|---|")
        for p in prof_files:
            m = re.match(r"(.*)_phases_j(\d+)\.txt", os.path.basename(p))
            txt = open(p).read()
            s = re.search(r"recursive rules \(join\+dedup\+insert\) ([\d.]+)%, merge new->full \(copy\) ([\d.]+)%, "
                          r"non-recursive rules ([\d.]+)%, loading ([\d.]+)%", txt)
            if s:
                other = float(s.group(3)) + float(s.group(4))
                md.append(f"| {m.group(1)} | {m.group(2)} | {s.group(1)}% | {s.group(2)}% | {other:.1f}% |")
    sc = os.path.join(RES, "profile", "scaling.csv")
    if os.path.exists(sc):
        rows = list(csv.DictReader(open(sc)))
        md.append("\n### Soufflé thread scaling (wall s / peak MB)\n")
        inputs = sorted({r["input"] for r in rows})
        md.append("| input | serial code | " + " | ".join(f"par j{t}" for t in (1, 2, 4, 8, 16)) + " |")
        md.append("|---|---|" + "---|" * 5)
        for inp in inputs:
            d = {(r["variant"], r["threads"]): r for r in rows if r["input"] == inp}
            def c(v, t):
                r = d.get((v, str(t)))
                return f"{float(r['wall_s']):.1f} / {r['maxrss_MB']}" if r and r["status"] == "0" else (r["status"] if r else "-")
            md.append(f"| {inp} | {c('souffle_seq', 1)} | " + " | ".join(c("souffle_par", t) for t in (1, 2, 4, 8, 16)) + " |")

    # ---------------- benchmarks ----------------
    med, status = load_bench()
    if med:
        inputs = []
        for (inp, _, _) in med:
            if inp not in inputs:
                inputs.append(inp)
        order = {"cspa": 0, "tc": 1, "sg": 2, "taint": 3}
        inputs.sort(key=lambda s: (order.get(s.split("_")[0], 9), s))
        md.append("\n### 1 thread: wall time in s (speedup of ours vs Soufflé)\n")
        md.append("| input | Soufflé serial | ours seq | speedup |")
        md.append("|---|---|---|---|")
        for inp in inputs:
            a, b = med.get((inp, "souffle_seq", 1)), med.get((inp, "seq", 1))
            sp = f"{a['wall'] / b['wall']:.2f}x" if a and b else "-"
            md.append(f"| {inp} | {cell(med, status, (inp, 'souffle_seq', 1))} | {cell(med, status, (inp, 'seq', 1))} | {sp} |")

        md.append("\n### 16 threads / 16 MPI ranks: wall time in s\n")
        engines = [e for e in ["souffle_par", "omp1", "omp2", "omp3", "mpi", "thrust"]
                   if any(k[1] == e and k[2] == 16 for k in med)]
        md.append("| input | " + " | ".join(ENGINE_NAME[e] for e in engines) + " | best ours vs Soufflé j16 | vs Soufflé serial |")
        md.append("|---|" + "---|" * (len(engines) + 2))
        speed_rows = []
        for inp in inputs:
            cells = [cell(med, status, (inp, e, 16)) for e in engines]
            s16 = med.get((inp, "souffle_par", 16))
            s1 = med.get((inp, "souffle_seq", 1))
            ours = [(med[(inp, e, 16)]["wall"], e) for e in engines[1:] if (inp, e, 16) in med]
            best = min(ours) if ours else None
            v16 = f"{s16['wall'] / best[0]:.2f}x ({ENGINE_NAME[best[1]]})" if s16 and best else "-"
            v1 = f"{s1['wall'] / best[0]:.1f}x" if s1 and best else "-"
            md.append(f"| {inp} | " + " | ".join(cells) + f" | {v16} | {v1} |")
            if s16:
                speed_rows.append((inp, {e: s16["wall"] / med[(inp, e, 16)]["wall"]
                                         for e in ("omp3", "mpi", "thrust") if (inp, e, 16) in med}))

        md.append("\n### Where our engines spend the solve time at 16 threads (s: join / sort+dedup / minus+merge / communication)\n")
        md.append("| input | " + " | ".join(ENGINE_NAME[e] for e in engines[1:]) + " |")
        md.append("|---|" + "---|" * (len(engines) - 1))
        for inp in inputs:
            cs = []
            for e in engines[1:]:
                m = med.get((inp, e, 16))
                cs.append(" / ".join(f"{x:.1f}" for x in m["split"]) if m else "-")
            md.append(f"| {inp} | " + " | ".join(cs) + " |")

        md.append("\n### Peak memory (MB) at 16 threads\n")
        md.append("| input | " + " | ".join(ENGINE_NAME[e] for e in engines) + " |")
        md.append("|---|" + "---|" * len(engines))
        for inp in inputs:
            md.append(f"| {inp} | " + " | ".join(
                f"{med[(inp, e, 16)]['rss']:.0f}" if (inp, e, 16) in med else "-" for e in engines) + " |")

        # chart: speedup vs Soufflé at 16 threads, per input
        if speed_rows:
            fig, ax = plt.subplots(figsize=(10, 4.2))
            n = len(speed_rows)
            width = 0.26
            shown = [e for e in ("omp3", "mpi", "thrust") if any(e in r[1] for r in speed_rows)]
            for j, e in enumerate(shown):
                xs = [i + (j - (len(shown) - 1) / 2) * width for i in range(n)]
                ys = [r[1].get(e, 0) for r in speed_rows]
                ax.bar(xs, ys, width=width - 0.03, color=COL[e], label=ENGINE_NAME[e])
            ax.axhline(1, color=INK2, linewidth=1, linestyle="--")
            ax.text(-0.6, 1.03, "Soufflé 16 threads = 1", ha="left", va="bottom", color=INK2, fontsize=8)
            ax.set_xticks(range(n), [r[0] for r in speed_rows], rotation=30, ha="right")
            ax.set_ylabel("speedup vs Soufflé (16 threads)")
            ax.set_title("16 threads: our engines vs Soufflé on the same rules and inputs (higher is better)", fontsize=10)
            ax.legend(frameon=False)
            ax.grid(axis="x", visible=False)
            fig.tight_layout()
            fig.savefig(os.path.join(RES, "speedup_vs_souffle_16.png"))
            plt.close(fig)

    out = os.path.join(RES, "summary.md")
    with open(out, "w") as f:
        f.write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
