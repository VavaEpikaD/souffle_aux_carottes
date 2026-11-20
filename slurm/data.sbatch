#!/bin/bash
#SBATCH --job-name=cspa-data
#SBATCH --partition=haswell
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=1700M
#SBATCH --time=00:09:30
set -euo pipefail
cd -- "${PROJECT_ROOT:-${SLURM_SUBMIT_DIR:-$PWD}}"
exec timeout --signal=TERM --kill-after=15s 520s python3 slurm/prepare_data.py "$@"
