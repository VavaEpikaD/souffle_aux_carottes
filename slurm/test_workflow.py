#!/usr/bin/env python3
"""Local checks for task matrices, data filtering and partial/failing summaries."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid
from cluster import ROOT, WORK, BACKENDS, atomic_json, sha256


def run(*arguments):
    return subprocess.run([sys.executable, *map(str, arguments)], cwd=ROOT,
                          check=True, text=True, capture_output=True)


manifest = json.loads((ROOT / 'slurm/generated/manifest.json').read_text())
for name, digest in manifest['sources'].items():
    assert sha256(ROOT / name) == digest
sources = {name: (ROOT / 'slurm/generated' / (name + '.cpp')).read_text() for name in BACKENDS}
assert 'PARALLEL_START' in sources['openmp']
assert 'mpi_runtime::owns' in sources['mpi']
assert 'pthread_runtime::parallelFor' in sources['pthreads']
assert 'mpi_runtime' not in sources['sequential'] and 'pthread_runtime' not in sources['sequential']

response = run('slurm/submit.py', 'all', '--sizes', '50000,200000', '--workers', '1,2,4',
               '--reps', '2', '--dry-run')
path = Path(next(line.split('Manifest: ', 1)[1] for line in response.stdout.splitlines()
                 if line.startswith('Manifest: ')))
tasks = json.loads(path.read_text())['tasks']
assert len(tasks) == 40
assert len({tuple(sorted(task.items())) for task in tasks}) == 40
assert all(task['workers'] == 1 for task in tasks if task['backend'] == 'sequential')
assert '--dependency=afterok:DRY_build:DRY_data' in response.stdout
assert '--time=00:09:30' in response.stdout
response = run('slurm/submit.py', 'bench', '--sizes', 'full', '--cpus', '32',
               '--mem', '48G', '--dry-run')
assert '--mem=48G' in response.stdout and '--mem-per-cpu' not in response.stdout
bad = subprocess.run([sys.executable, 'slurm/submit.py', 'bench', '--timeout', '540', '--dry-run'],
                     cwd=ROOT, capture_output=True)
assert bad.returncode != 0

with tempfile.TemporaryDirectory() as temp:
    source = Path(temp)
    (source / 'assign.facts').write_text('0\t1\n1\t7\n6\t6\n')
    (source / 'dereference.facts').write_text('7\t0\n2\t3\n')
    run('slurm/prepare_data.py', '--source', source, '--sizes', '7')
    data = json.loads((WORK / 'data/7.json').read_text())
    assert data['counts'] == {'assign': 2, 'dereference': 1}
    assert (Path(data['facts']) / 'assign.facts').read_text() == '0\t1\n6\t6\n'

run_id = 'selftest-' + uuid.uuid4().hex[:8]
atomic_json(WORK / 'manifests' / (run_id + '.json'), {'tasks': [1, 2, 3, 4]})
base = dict(dataset='test', workers=1, counts={'ValueFlow': 1}, status='ok',
            wall_s=10., sampled_peak_total_rss_mib=1.)
records = [dict(base, backend='sequential'),
           dict(base, backend='pthreads', counts={'ValueFlow': 2}, wall_s=5.),
           dict(base, backend='mpi', status='timeout', wall_s=480.)]
for index, record in enumerate(records):
    atomic_json(WORK / 'results' / run_id / ('%05d.json' % index), record)
run('slurm/summarize.py', run_id)
summary = json.loads((WORK / 'results' / run_id / 'summary.json').read_text())
assert summary['missing'] == 1
assert {row['status'] for row in summary['failures']} == {'wrong-counts', 'timeout'}
assert all(row['median_s'] is None for row in summary['summary'] if row['backend'] != 'sequential')
for backend, body, expected_status in (
        ('selftest-fast', 'printf "MemoryAlias\\t1\\nValueAlias\\t2\\nValueFlow\\t3\\n"', 'ok'),
        ('selftest-slow', 'sleep 3', 'timeout')):
    binary = WORK / 'bin' / backend
    binary.parent.mkdir(parents=True, exist_ok=True)
    binary.write_text('#!/bin/sh\n' + body + '\n')
    binary.chmod(0o755)
    atomic_json(binary.with_suffix('.json'), {'test': True})
    task = dict(backend=backend, workers=1, dataset='7', rep=1)
    task_manifest = WORK / 'manifests' / (backend + '.json')
    atomic_json(task_manifest, dict(run_id=backend, tasks=[task], timeout=1, omp_wait='default'))
    process = subprocess.run([sys.executable, 'slurm/benchmark.py', str(task_manifest)],
                             cwd=ROOT, text=True, capture_output=True)
    record = json.loads((WORK / 'results' / backend / '00000.json').read_text())
    assert record['status'] == expected_status, (record, process.stderr)
    assert process.returncode == (0 if expected_status == 'ok' else 124)
    assert record['wall_s'] > 0
print('PASS source provenance, matrices/dependencies, resource bounds, filtering, failure/partial summaries, execution and timeout recording')
