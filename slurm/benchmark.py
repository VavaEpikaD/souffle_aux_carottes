#!/usr/bin/env python3
"""One backend/input/worker/repetition per Slurm array task; no compilation."""
import argparse
import json
import os
from pathlib import Path
import shlex
import socket
import subprocess
import threading
import time
from cluster import ROOT, WORK, atomic_json, environment_command


def samples(pid, binary):
    pending = [pid]
    memory, threads = [], []
    while pending:
        current = pending.pop()
        try:
            pending.extend(int(x) for x in Path('/proc/%s/task/%s/children' % (current, current)).read_text().split())
            if Path(os.readlink('/proc/%s/exe' % current)).name != binary:
                continue
            status = Path('/proc/%s/status' % current).read_text().splitlines()
            memory.append(next(int(line.split()[1]) for line in status if line.startswith('VmRSS:')))
            threads.append(next(int(line.split()[1]) for line in status if line.startswith('Threads:')))
        except (OSError, StopIteration):
            continue
    return len(memory), sum(memory), max(memory, default=0), sum(threads)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--index', type=int, default=int(os.environ.get('SLURM_ARRAY_TASK_ID', '0')))
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    task = manifest['tasks'][args.index]
    backend, workers, dataset = task['backend'], task['workers'], task['dataset']
    allocated = int(os.environ.get('SLURM_CPUS_PER_TASK', workers))
    if workers > allocated:
        raise SystemExit('Requested workers exceed allocated CPUs')
    data = json.loads((WORK / 'data' / (dataset + '.json')).read_text())
    facts = Path(data['facts'])
    binary = WORK / 'bin' / backend
    build = json.loads(binary.with_suffix('.json').read_text())
    output = WORK / 'results' / manifest['run_id']
    output.mkdir(parents=True, exist_ok=True)
    stem = output / ('%05d' % args.index)
    application = [str(binary), '-F', str(facts)]
    if backend in ('pthreads', 'openmp'):
        application += ['-j', str(workers)]
    if backend == 'mpi':
        # Same container supplies compiler and MPI runtime. Keep these jobs on
        # one allocated node; avoid host/container PMIx or MPI ABI mismatches.
        application = ['mpirun', '--mca', 'ras', '^slurm', '--mca', 'plm', 'isolated',
                       '--host', 'localhost:' + str(workers), '--oversubscribe',
                       '--bind-to', 'none', '-np', str(workers), *application]
    command = ['timeout', '--signal=TERM', '--kill-after=15s', str(manifest['timeout']) + 's',
               *environment_command(application, facts=facts)]
    env = dict(os.environ, OMP_NUM_THREADS=str(workers), OMP_DYNAMIC='FALSE', OMP_PROC_BIND='FALSE')
    if manifest['omp_wait'] != 'default':
        env['OMP_WAIT_POLICY'] = manifest['omp_wait'].upper()
    for name in ('OMP_NUM_THREADS', 'OMP_DYNAMIC', 'OMP_PROC_BIND', 'OMP_WAIT_POLICY'):
        if name in env:
            env['APPTAINERENV_' + name] = env[name]
    print('Task:', task, '\nCommand:', shlex.join(command), flush=True)
    peaks = [0, 0, 0, 0]
    stop = threading.Event()
    start = time.monotonic()
    with Path(str(stem) + '.stdout').open('w') as stdout, Path(str(stem) + '.stderr').open('w') as stderr:
        process = subprocess.Popen(command, stdout=stdout, stderr=stderr, env=env)
        def monitor():
            while not stop.wait(.1):
                for index, value in enumerate(samples(process.pid, backend)):
                    peaks[index] = max(peaks[index], value)
        monitor_thread = threading.Thread(target=monitor, daemon=True)
        monitor_thread.start()
        process.wait()
        wall = time.monotonic() - start
        stop.set()
        monitor_thread.join()
    peak_count, peak_total, peak_process, peak_threads = peaks
    counts = {}
    for line in Path(str(stem) + '.stdout').read_text().splitlines():
        fields = line.split()
        if len(fields) == 2 and fields[1].isdigit():
            counts[fields[0]] = int(fields[1])
    expected = {'200000': {'MemoryAlias': 10778093, 'ValueAlias': 28256054, 'ValueFlow': 121505}}
    code = process.returncode
    status = 'ok' if code == 0 else ('timeout' if code == 124 else ('killed' if code == 137 else 'error'))
    if status == 'ok' and (set(counts) != {'MemoryAlias', 'ValueAlias', 'ValueFlow'} or
                           (dataset in expected and counts != expected[dataset])):
        status = 'wrong-counts'
    result = dict(task, status=status, exit=code, wall_s=round(wall, 3), counts=counts,
                  sampled_peak_total_rss_mib=round(peak_total / 1024, 1),
                  sampled_peak_process_rss_mib=round(peak_process / 1024, 1),
                  peak_processes=peak_count, peak_threads=peak_threads,
                  build=build, input=data, command=command, node=socket.gethostname(),
                  job_id=os.environ.get('SLURM_JOB_ID'), array_index=args.index,
                  allocated_cpus=allocated, omp_wait=env.get('OMP_WAIT_POLICY', 'default'),
                  timestamp=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
    atomic_json(stem.with_suffix('.json'), result)
    print(json.dumps(result, indent=2), flush=True)
    return 0 if status == 'ok' else (124 if status == 'timeout' else 1)


if __name__ == '__main__':
    raise SystemExit(main())
