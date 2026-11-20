#!/bin/bash
# resume.sh - continue the measurements after an interruption (our code)
#
# usage (inside WSL, keep a terminal open so WSL does not shut down):
#   cd parallel_datalog && bash scripts/resume.sh >> results/run_all.log 2>&1
#
# Same work as run_all.sh, but skips what is already in results/:
#   - profile inputs whose 16-thread Soufflé run is already in results/profile/scaling.csv
#   - benchmark configurations already in results/bench/bench.csv (bench.py skips them itself)
cd "$(dirname "$0")/.."
export OMP_WAIT_POLICY=passive
echo "resume $(date)"
todo=""
for n in cspa_httpd tc_SF.cedge sg_ego-Facebook taint_p2p-Gnutella04; do
    r=${n%%_*}
    grep -q "^$r,$n,souffle_par,16," results/profile/scaling.csv 2>/dev/null || todo="$todo $n"
done
[ -n "$todo" ] && SCALING_THREADS="1 16" bash scripts/profile_souffle.sh $todo
echo "profile done $(date)"
python3 scripts/bench.py --threads 1,16 --inputs cspa_bzip2,cspa_lua,cspa_linux,cspa_httpd,cspa_postgresql,tc_p2p-Gnutella04,tc_SF.cedge,tc_fe_body,sg_ego-Facebook,sg_CA-HepTh,taint_httpd,taint_p2p-Gnutella04,taint_fe_body
echo "bench done $(date)"
