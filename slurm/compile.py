#!/usr/bin/env python3
"""Compile one already-generated CSPA backend in a bounded batch job."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import time
from cluster import ROOT, WORK, BACKENDS, atomic_json, environment_command, sha256

parser = argparse.ArgumentParser()
parser.add_argument('backend', choices=BACKENDS + ('array',))
args = parser.parse_args()
backend = BACKENDS[int(os.environ['SLURM_ARRAY_TASK_ID'])] if args.backend == 'array' else args.backend
source = ROOT / 'slurm/generated' / (backend + '.cpp')
include = ROOT / ('slurm/upstream/src/include' if backend == 'openmp' else 'soufle/src/include')
binary = WORK / 'bin' / backend
binary.parent.mkdir(parents=True, exist_ok=True)
temporary = binary.with_name(backend + '.tmp-' + os.environ.get('SLURM_JOB_ID', str(os.getpid())))
cxx = os.environ.get('MPICXX' if backend == 'mpi' else 'CXX', 'mpicxx' if backend == 'mpi' else 'g++')
flags = ['-std=c++17', '-O3', '-DNDEBUG', '-pthread', '-DRAM_DOMAIN_SIZE=64']
if backend == 'openmp':
    flags += ['-fopenmp']
command = environment_command([cxx, *flags, '-I' + str(include), source, '-o', temporary])
print('Compiling:', shlex.join(command), flush=True)
start = time.monotonic()
result = subprocess.run(['timeout', '--signal=TERM', '--kill-after=15s', '480s', *command])
if result.returncode:
    print('Compile stopped with code', result.returncode, '; resubmit this backend.', flush=True)
    raise SystemExit(result.returncode)
temporary.replace(binary)
version = subprocess.run(environment_command([cxx, '--version']), check=True,
                         capture_output=True, text=True).stdout.splitlines()[0]
atomic_json(binary.with_suffix('.json'), dict(backend=backend, flags=flags,
            compiler=version, source_sha256=sha256(source), binary_sha256=sha256(binary),
            compile_seconds=round(time.monotonic() - start, 3), command=command,
            node=os.uname().nodename, job_id=os.environ.get('SLURM_JOB_ID')))
print('Ready:', binary, flush=True)
