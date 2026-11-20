#!/bin/bash
# run_all.sh - the whole measurement: Soufflé profile, then all benchmarks (our code)
# usage: bash scripts/run_all.sh > results/run_all.log 2>&1
cd "$(dirname "$0")/.."
export OMP_WAIT_POLICY=passive          # see scripts/bench.py
echo "start $(date)"
# 1. profile: one mid-size input per rule set (+ httpd), thread sweep only on CSPA linux
SCALING_THREADS="1 2 4 8 16" bash scripts/profile_souffle.sh cspa_linux
SCALING_THREADS="1 16" bash scripts/profile_souffle.sh cspa_httpd tc_SF.cedge sg_ego-Facebook taint_p2p-Gnutella04
echo "profile done $(date)"
# 2. benchmarks at 1 and 16 threads
python3 scripts/bench.py --threads 1,16 --inputs cspa_bzip2,cspa_lua,cspa_linux,cspa_httpd,cspa_postgresql,tc_p2p-Gnutella04,tc_SF.cedge,tc_fe_body,sg_ego-Facebook,sg_CA-HepTh,taint_httpd,taint_p2p-Gnutella04,taint_fe_body
echo "bench done $(date)"
