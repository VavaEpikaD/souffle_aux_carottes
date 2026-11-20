#!/bin/bash
#SBATCH --job-name=cspa-bench
#SBATCH --partition=haswell
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --time=00:09:30
set -euo pipefail
cd -- "${PROJECT_ROOT:-${SLURM_SUBMIT_DIR:-$PWD}}"
exec timeout --signal=TERM --kill-after=15s 520s python3 slurm/benchmark.py "$@"
