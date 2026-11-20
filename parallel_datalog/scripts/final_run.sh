#!/bin/bash
# final_run.sh - the measurements that fit in the remaining time, most important first (our code)
cd "$(dirname "$0")/.."
export OMP_WAIT_POLICY=passive
echo "final start $(date)"
# 1. benchmark: 1 thread = Soufflé serial vs ours seq; 16 threads = Soufflé parallel vs our OpenMP / MPI
python3 scripts/bench.py --threads 16 --engines souffle_seq,seq,souffle_par,omp1,omp2,omp3,mpi \
  --inputs cspa_bzip2,cspa_lua,cspa_linux,tc_SF.cedge,taint_httpd,taint_p2p-Gnutella04,cspa_httpd,sg_ego-Facebook
echo "bench core done $(date)"
# 2. profile of the remaining rule sets
SCALING_THREADS="16" bash scripts/profile_souffle.sh taint_p2p-Gnutella04 sg_ego-Facebook
echo "profile done $(date)"
# 3. extras: our best version on 1 thread (algorithm gain vs parallel gain), one more big input
python3 scripts/bench.py --threads 1 --engines omp3 --inputs cspa_linux,tc_SF.cedge,taint_p2p-Gnutella04,cspa_httpd
python3 scripts/bench.py --threads 16 --engines souffle_seq,seq,souffle_par,omp3,mpi --inputs cspa_postgresql
echo "all done $(date)"
