#!/usr/bin/env python3
"""Submit bounded build/data/benchmark arrays with optional dependencies."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import time
import uuid
from cluster import ROOT, WORK, BACKENDS, atomic_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('build', 'data', 'bench', 'all'))
    parser.add_argument('--backends', default=','.join(BACKENDS))
    parser.add_argument('--sizes', default='200000', help='endpoint limits, e.g. 50000,200000,500000,full')
    parser.add_argument('--workers', default='4', help='comma-separated counts; sequential always uses 1')
    parser.add_argument('--reps', type=int, default=3)
    parser.add_argument('--cpus', type=int, help='allocated CPUs per benchmark task; defaults to max worker count')
    parser.add_argument('--mem', help='total memory per benchmark task, e.g. 48G (otherwise 1700M per CPU)')
    parser.add_argument('--partition', default='haswell')
    parser.add_argument('--concurrency', type=int, default=1)
    parser.add_argument('--exclusive', action='store_true', help='reserve separate nodes for benchmark tasks')
    parser.add_argument('--timeout', type=int, default=480, help='per-run seconds, maximum 480')
    parser.add_argument('--facts', type=Path, default=Path.home() / 'stadata/cspa/postgresql')
    parser.add_argument('--omp-wait', choices=('default', 'passive', 'active'), default='default')
    parser.add_argument('--after', help='existing job IDs, colon-separated, required to finish successfully')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    backends = list(dict.fromkeys(args.backends.split(',')))
    sizes = list(dict.fromkeys(args.sizes.split(',')))
    try:
        workers = sorted(set(map(int, args.workers.split(','))))
        for size in sizes:
            if size != 'full' and int(size) <= 0:
                raise ValueError()
    except ValueError:
        parser.error('sizes must be positive integers/full; workers must be positive integers')
    if not backends or any(x not in BACKENDS for x in backends):
        parser.error('backends must be sequential,pthreads,openmp,mpi')
    if not workers or min(workers) < 1 or args.reps < 1 or args.concurrency < 1:
        parser.error('workers, reps and concurrency must be positive')
    if not 1 <= args.timeout <= 480:
        parser.error('timeout must be between 1 and 480 seconds')
    cpus = args.cpus or max(workers)
    if args.cpus is not None and args.cpus < 1:
        parser.error('--cpus must be positive')
    if cpus < max(workers):
        parser.error('--cpus must be at least the largest worker count')
    run_id = time.strftime('%Y%m%d-%H%M%S') + '-' + uuid.uuid4().hex[:6]
    logs = WORK / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    manifest_path = WORK / 'manifests' / (run_id + '.json')
    tasks = [dict(backend=backend, dataset=size, workers=count, rep=rep)
             for size in sizes for backend in backends
             for count in ([1] if backend == 'sequential' else workers)
             for rep in range(1, args.reps + 1)]
    manifest = dict(run_id=run_id, tasks=tasks, timeout=args.timeout, omp_wait=args.omp_wait,
                    partition=args.partition, allocated_cpus=cpus, memory=args.mem,
                    container=os.environ.get('USE_CONTAINER', 'auto'),
                    sif=os.environ.get('SIF'), exclusive=args.exclusive)
    atomic_json(manifest_path, manifest)
    jobs = {}

    def submit(label, script, options, arguments, dependencies=()):
        command = ['sbatch', '--parsable', '--partition=' + args.partition,
                   '--time=00:09:30', '--nodes=1', '--ntasks=1', '--export=ALL',
                   '--chdir=' + str(ROOT), '--output=' + str(logs / '%x-%A_%a.out')]
        if dependencies:
            command += ['--dependency=afterok:' + ':'.join(dependencies)]
        command += options + [str(ROOT / 'slurm' / script)] + list(map(str, arguments))
        print(shlex.join(command), flush=True)
        if args.dry_run:
            result = 'DRY_' + label
        else:
            response = subprocess.run(command, cwd=ROOT, check=True, text=True, capture_output=True)
            result = response.stdout.strip().split(';')[0]
            if not result.isdigit():
                raise RuntimeError('Unexpected sbatch response: ' + response.stdout)
        jobs[label] = result
        return result

    dependencies = args.after.split(':') if args.after else []
    if args.action in ('build', 'all'):
        indices = ','.join(str(BACKENDS.index(x)) for x in backends)
        job = submit('build', 'compile.sbatch', ['--array=' + indices + '%' + str(args.concurrency)],
                     ['array'], dependencies)
        dependencies.append(job)
    if args.action in ('data', 'all'):
        job = submit('data', 'data.sbatch', [], ['--source', args.facts.resolve(), '--sizes', ','.join(sizes)],
                     args.after.split(':') if args.after else [])
        dependencies.append(job)
    if args.action in ('bench', 'all'):
        options = ['--cpus-per-task=' + str(cpus),
                   '--array=0-' + str(len(tasks) - 1) + '%' + str(args.concurrency)]
        if args.mem:
            options += ['--mem=' + args.mem]
        else:
            options += ['--mem-per-cpu=1700M']
        if args.exclusive:
            options += ['--exclusive']
        submit('bench', 'benchmark.sbatch', options, [manifest_path], dependencies)
    atomic_json(manifest_path.with_suffix('.submission.json'), dict(jobs=jobs, dry_run=args.dry_run))
    print('Manifest:', manifest_path)
    print('Jobs:', json.dumps(jobs))
    if args.action in ('bench', 'all'):
        print('Summarize: python3 slurm/summarize.py ' + run_id)


if __name__ == '__main__':
    main()
