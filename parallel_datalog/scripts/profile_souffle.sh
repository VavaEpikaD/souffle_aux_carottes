#!/bin/bash
# profile_souffle.sh - where does Soufflé spend its time? (our code)
#
# usage: bash scripts/profile_souffle.sh input1 input2 ...   (names of dirs in $STADATA/inputs,
#        e.g. cspa_linux tc_SF.cedge; the rule set is the part before the first "_")
#
# For every input, into results/profile/:
#   1. <input>_phases_j1.txt / _j16.txt   Soufflé's own profiler (-p): time in the recursive rules
#      (join + dedup check + insert into `new`), in the copy of `new` into the full relation
#      (serial merge), per iteration + new tuples per iteration (<input>_iters_j*.csv)
#   2. <input>_perf_j1.txt / _j16.txt     perf (cpu-clock sampling): hottest functions
#      (B-tree insert / contains / lower_bound, OpenMP runtime, ...)
#   3. scaling.csv                        wall time + peak memory for serial code and for the
#      parallel code at 1, 2, 4, 8, 16 threads
cd "$(dirname "$0")/.."
DATA=${STADATA:-$HOME/stadata}/inputs
OUT=results/profile
PERF=$(ls /usr/lib/linux-tools/*/perf | head -1)
TMO=${TMO:-1800}
mkdir -p "$OUT" bin
[ -f "$OUT/scaling.csv" ] || echo "rules,input,variant,threads,wall_s,maxrss_MB,status" > "$OUT/scaling.csv"

timed() {   # timed <rules> <input> <variant> <threads> <cmd...>
    local r=$1 n=$2 var=$3 t=$4
    shift 4
    /usr/bin/time -f "%e %M" -o /tmp/prof_time timeout $TMO "$@" > /dev/null 2>&1
    local st=$?
    read -r wall rss < /tmp/prof_time
    [ $st -eq 124 ] && st=timeout
    echo "$r,$n,$var,$t,$wall,$((rss / 1024)),$st" | tee -a "$OUT/scaling.csv"
}

for n in "$@"; do
    r=${n%%_*}
    d=$DATA/$n
    [ -d "$d" ] || { echo "no input $d"; continue; }
    # profiling build (parallel code + Soufflé's profiler); it writes prof.log in the cwd
    [ -x bin/souffle_${r}_prof ] || souffle -w -j 16 -M "SIZE_ONLY" -p prof.log -o bin/souffle_${r}_prof rules/$r.dl
    for t in 1 16; do
        w=$(mktemp -d)
        (cd "$w" && timeout $TMO "$OLDPWD/bin/souffle_${r}_prof" -F "$d" -j $t > /dev/null 2>&1)
        python3 tools/parse_souffleprof.py "$w/prof.log" --iters "$OUT/${n}_iters_j$t.csv" > "$OUT/${n}_phases_j$t.txt"
        echo "== $n j$t"; cat "$OUT/${n}_phases_j$t.txt"
        rm -rf "$w"
        # hotspots
        $PERF record -q -e cpu-clock -F 999 -o /tmp/perf.data -- timeout $TMO bin/souffle_${r}_par -F "$d" -j $t > /dev/null 2>&1
        $PERF report -i /tmp/perf.data --no-children --sort symbol --stdio 2>/dev/null |
            grep -v "^#" | grep -v "^$" | head -30 | cut -c1-160 > "$OUT/${n}_perf_j$t.txt"
        head -8 "$OUT/${n}_perf_j$t.txt"
    done
    # scaling
    timed $r $n souffle_seq 1 bin/souffle_${r}_seq -F "$d"
    for t in ${SCALING_THREADS:-1 2 4 8 16}; do
        timed $r $n souffle_par $t bin/souffle_${r}_par -F "$d" -j $t
    done
done
