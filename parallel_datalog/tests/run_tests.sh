#!/bin/bash
# run_tests.sh - correctness of every engine (our code)
#
# For every case in tests/make_cases.py:
#   1. Soufflé must give the hand-written expected output (when there is one)
#   2. every engine / version / thread count must give exactly Soufflé's output
#      (compared as size + order-independent hash of every output relation)
# usage: bash tests/run_tests.sh       (after `make`)
cd "$(dirname "$0")/.." || exit 1
T=${TMPDIR:-/tmp}/sta_tests
rm -rf "$T"
mkdir -p "$T"
python3 tests/make_cases.py "$T/cases" || exit 1
export OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1
MPIRUN=${MPIRUN:-mpirun --oversubscribe}

pass=0
fail=0
norm() { tr '|' '\n' | sort | tr '\n' '|'; }

check() {   # check <label> <reference string> <command...>
    local label=$1 ref=$2
    shift 2
    local got
    got=$("$@" 2> "$T/err" | awk -F, '{print $NF}' | norm)
    if [ "$got" = "$ref" ]; then
        pass=$((pass + 1))
    else
        echo "FAIL $label"
        echo "     expected $ref"
        echo "     got      $got $(head -c 200 "$T/err")"
        fail=$((fail + 1))
    fi
}

for d in "$T"/cases/*/; do
    c=$(basename "$d")
    r=$(cat "$d/rules")
    rm -rf "$T/souffle"
    mkdir -p "$T/souffle"
    bin/souffle_$r -F "$d/facts" -D "$T/souffle"
    rels=$(ls "$T/souffle" | sed 's/\.csv$//')
    ref=$(bin/relhash "$T/souffle" $rels | norm)
    if [ -d "$d/expected" ]; then
        exp=$(bin/relhash "$d/expected" $(ls "$d/expected" | sed 's/\.csv$//') | norm)
        if [ "$exp" = "$ref" ]; then pass=$((pass + 1)); else echo "FAIL $c souffle vs hand-written"; fail=$((fail + 1)); fi
    fi
    echo "case $c ($r): $ref"
    check "$c seq" "$ref" bin/seq -r $r -F "$d/facts"
    for v in 1 2 3; do
        for t in 1 4; do
            check "$c omp v$v t$t" "$ref" bin/omp -v $v -t $t -r $r -F "$d/facts"
        done
    done
    for t in 1 4; do
        check "$c thrust_omp t$t" "$ref" bin/thrust_omp -t $t -r $r -F "$d/facts"
    done
    if [ -x bin/mpi ]; then
        for np in 1 3; do
            check "$c mpi np$np" "$ref" $MPIRUN -np $np bin/mpi -r $r -F "$d/facts"
        done
    fi
    if [ -x bin/thrust_gpu ] && command -v nvidia-smi > /dev/null; then
        check "$c thrust_gpu" "$ref" bin/thrust_gpu -r $r -F "$d/facts"
    fi
done
# Bigger input (if downloaded): the tiny cases never reach the parallel merge / difference code
# (it starts at 200k facts). A bug there (first piece of the merge started after B's smallest facts)
# passed all tiny cases but gave wrong facts on CSPA lua - this check would have caught it.
BIG=${STADATA:-$HOME/stadata}/inputs/cspa_lua
if [ -d "$BIG" ]; then
    ref=$(bin/seq -r cspa -F "$BIG" | awk -F, '{print $NF}' | norm)
    echo "big case cspa_lua: $ref"
    for v in 1 2 3; do
        check "cspa_lua omp v$v t16" "$ref" bin/omp -v $v -t 16 -r cspa -F "$BIG"
    done
    if [ -x bin/mpi ]; then check "cspa_lua mpi np8" "$ref" $MPIRUN -np 8 bin/mpi -r cspa -F "$BIG"; fi
fi

echo
echo "passed: $pass   failed: $fail"
[ $fail -eq 0 ]
