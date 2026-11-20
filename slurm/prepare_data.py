#!/usr/bin/env python3
"""Stream the server's PostgreSQL facts into deterministic induced subsets."""
import argparse
import os
from pathlib import Path
from cluster import ROOT, WORK, atomic_json, sha256

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=Path.home() / 'stadata/cspa/postgresql')
parser.add_argument('--sizes', default='200000')
args = parser.parse_args()
sizes = sorted(set(args.sizes.split(',')), key=lambda x: (x == 'full', int(x) if x != 'full' else 0))
if any(size != 'full' and int(size) <= 0 for size in sizes):
    parser.error('sizes must be positive numeric endpoint limits, or full')
for size in sizes:
    # Ship the exact previously benchmarked subset; no need to regenerate it.
    prepared = ROOT / 'slurm/inputs' / size
    source = prepared if (prepared / 'assign.facts').is_file() else args.source.resolve()
    target = source if size == 'full' or source == prepared else WORK / 'data' / size
    target.mkdir(parents=True, exist_ok=True)
    counts = {}
    hashes = {}
    for name in ('assign', 'dereference'):
        original = source / (name + '.facts')
        if not original.is_file():
            raise SystemExit('Missing input: ' + str(original))
        count = 0
        if target == source:
            with original.open('rb') as stream:
                count = sum(1 for line in stream if line.strip())
        else:
            output = target / (name + '.facts')
            temporary = output.with_name(output.name + '.tmp-' + str(os.getpid()))
            with original.open() as stream, temporary.open('w') as dest:
                for line in stream:
                    if not line.strip():
                        continue
                    a, b = map(int, line.split())
                    if a < int(size) and b < int(size):
                        dest.write(line)
                        count += 1
            temporary.replace(output)
        counts[name] = count
        hashes[name] = sha256(target / (name + '.facts'))
    record = dict(dataset=size, facts=str(target), counts=counts, sha256=hashes,
                  rule='both endpoints below limit' if size != 'full' else 'full input')
    atomic_json(WORK / 'data' / (size + '.json'), record)
    print(record, flush=True)
