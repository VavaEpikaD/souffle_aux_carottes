#!/usr/bin/env python3
"""bench.py - run Soufflé and our engines on the benchmark inputs (our code).

usage: python3 scripts/bench.py [--inputs cspa_linux,tc_SF.cedge,...] [--threads 1,16]
                               [--engines souffle_seq,souffle_par,seq,omp1,omp2,omp3,mpi,thrust]
                               [--out results/bench/bench.csv] [--timeout 1800]

Every run is one process: wall time (incl. reading the facts) from /usr/bin/time, peak memory,
and for our engines also the solve time and the per-operator split they print.
- Runs shorter than 60 s are repeated 3 times (the plots use the median).
- Correctness: the sizes of all output relations must equal Soufflé's, and every engine's
  order-independent hash must equal our sequential engine's (which equals Soufflé's on all
  test cases, see tests/run_tests.sh). A mismatch is recorded as status WRONG.
- A run that hits the timeout or the memory limit is recorded (status timeout / oom) and that
  engine is not run on bigger thread counts of the same input.
"""
import argparse
import csv
import os
import re
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.environ.get("STADATA", os.path.expanduser("~/stadata")) + "/inputs"
MEMLIMIT = 20 * 1024 ** 3      # 20 GB address space per run (WSL has 23 GB)
# OMP_WAIT_POLICY=passive: with 16 threads on 16 hardware threads, active spinning at barriers
# made runs vary 2x (any other process stalls one thread, 15 spin). Used for Soufflé AND us.
ENV = dict(os.environ, OMPI_ALLOW_RUN_AS_ROOT="1", OMPI_ALLOW_RUN_AS_ROOT_CONFIRM="1", OMP_WAIT_POLICY="passive")

FIELDS = ["rules", "input", "engine", "version", "threads", "rep", "wall_s", "solve_s", "load_s",
          "maxrss_MB", "iters", "join_s", "sort_s", "diff_s", "comm_s", "sizes", "hash", "status"]


def command(engine, rules, inp, t):
    d = os.path.join(DATA, inp)
    b = os.path.join(ROOT, "bin")
    if engine == "souffle_seq":
        return [f"{b}/souffle_{rules}_seq", "-F", d], "v1"
    if engine == "souffle_par":
        return [f"{b}/souffle_{rules}_par", "-F", d, "-j", str(t)], "v1"
    if engine == "seq":
        return [f"{b}/seq", "-r", rules, "-F", d], "v1"
    if engine in ("omp1", "omp2", "omp3"):
        return [f"{b}/omp", "-v", engine[-1], "-t", str(t), "-r", rules, "-F", d], "v" + engine[-1]
    if engine == "mpi":
        return ["mpirun", "--use-hwthread-cpus", "--oversubscribe", "-np", str(t), f"{b}/mpi",
                "-r", rules, "-F", d], "v1"
    if engine == "thrust":
        return [f"{b}/thrust_omp", "-t", str(t), "-r", rules, "-F", d], "v1"
    if engine == "thrust_gpu":      # Colab: the same Thrust code compiled with nvcc
        return [f"{b}/thrust_gpu", "-r", rules, "-F", d], "v1"
    raise ValueError(engine)


def run_once(cmd, timeout):
    """returns (status, wall, maxrss_MB, stdout)"""
    full = ["prlimit", f"--as={MEMLIMIT}", "/usr/bin/time", "-f", "TIME %e %M", "timeout", str(timeout)] + cmd
    p = subprocess.run(full, capture_output=True, text=True, env=ENV, cwd=ROOT)
    m = re.findall(r"TIME ([\d.]+) (\d+)", p.stderr)
    wall, rss = (float(m[-1][0]), int(m[-1][1]) / 1024) if m else (0.0, 0.0)
    if p.returncode == 124:
        return "timeout", wall, rss, p.stdout
    if p.returncode != 0:
        err = p.stderr.lower()
        st = "oom" if ("bad_alloc" in err or "memory" in err or p.returncode in (134, 137, -9)) else f"error{p.returncode}"
        return st, wall, rss, p.stdout
    return "ok", wall, rss, p.stdout


def parse(engine, out):
    """sizes {rel: n}, hash string, and our engines' extra columns"""
    row = {}
    sizes = {}
    if engine.startswith("souffle"):
        for line in out.splitlines():
            p = line.split()
            if len(p) == 2 and p[1].isdigit():
                sizes[p[0]] = int(p[1])
        return sizes, "", row
    line = [l for l in out.splitlines() if l.count(",") >= 13][-1]
    c = line.split(",")
    row.update(load_s=c[5], solve_s=c[6], iters=c[7], join_s=c[8], sort_s=c[9], diff_s=c[10], comm_s=c[11])
    hashes = []
    for part in c[13].split("|"):
        name, rest = part.split("=")
        n, h = rest.split(":")
        sizes[name] = int(n)
        hashes.append(f"{name}:{h}")
    return sizes, ";".join(sorted(hashes)), row


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inputs", required=True)
    ap.add_argument("--threads", default="1,16")
    ap.add_argument("--engines", default="souffle_seq,souffle_par,seq,omp1,omp2,omp3,mpi,thrust")
    ap.add_argument("--out", default=os.path.join(ROOT, "results/bench/bench.csv"))
    ap.add_argument("--timeout", type=int, default=1800)
    ap.add_argument("--reps", type=int, default=3)
    a = ap.parse_args()
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    new = not os.path.exists(a.out)
    # resume: configurations already in the CSV are not run again; their sizes / hashes are
    # still used as the reference for the correctness check of the remaining runs
    done, old_sizes, old_hash = set(), {}, {}
    if not new:
        for row in csv.DictReader(open(a.out)):
            done.add((row["input"], row["engine"], int(row["threads"])))
            if row["status"] == "ok":
                sz = dict(x.split("=") for x in row["sizes"].split(";") if x)
                old_sizes.setdefault(row["input"], {k: int(v) for k, v in sz.items()})
                if row["hash"]:
                    old_hash.setdefault(row["input"], row["hash"])
    f = open(a.out, "a", newline="")
    w = csv.DictWriter(f, fieldnames=FIELDS)
    if new:
        w.writeheader()
    threads = [int(x) for x in a.threads.split(",")]
    for inp in a.inputs.split(","):
        rules = inp.split("_")[0]
        ref_sizes, ref_hash = old_sizes.get(inp), old_hash.get(inp)
        for engine in a.engines.split(","):
            single = engine in ("souffle_seq", "seq")
            for t in ([1] if single else threads):
                if (inp, engine, t) in done:
                    continue
                cmd, ver = command(engine, rules, inp, t)
                reps = a.reps
                r = 0
                while r < reps:
                    st, wall, rss, out = run_once(cmd, a.timeout)
                    row = dict(rules=rules, input=inp, engine=engine, version=ver, threads=t, rep=r + 1,
                               wall_s=wall, maxrss_MB=round(rss), status=st)
                    if st == "ok":
                        sizes, h, extra = parse(engine, out)
                        row.update(extra)
                        row["sizes"] = ";".join(f"{k}={v}" for k, v in sorted(sizes.items()))
                        row["hash"] = h
                        # ref sizes from the first engine (souffle), ref hash from our seq engine
                        if ref_sizes is None:
                            ref_sizes = sizes
                        elif {k: sizes.get(k) for k in ref_sizes} != ref_sizes:
                            row["status"] = st = "WRONG"
                        if h:
                            if ref_hash is None:
                                ref_hash = h
                            elif h != ref_hash:
                                row["status"] = st = "WRONG"
                    w.writerow(row)
                    f.flush()
                    print(",".join(str(row.get(k, "")) for k in FIELDS), flush=True)
                    if st != "ok" or wall >= 60:
                        break
                    r += 1
                if st not in ("ok", "WRONG"):
                    break       # timeout / oom: skip larger thread counts of this engine
    f.close()


if __name__ == "__main__":
    main()
