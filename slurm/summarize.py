#!/usr/bin/env python3
"""Aggregate completed records, preserving failures and checking result counts."""
import argparse
import csv
from collections import defaultdict
import json
from pathlib import Path
import statistics
from cluster import WORK, atomic_json

parser = argparse.ArgumentParser()
parser.add_argument('run_id')
args = parser.parse_args()
manifest = json.loads((WORK / 'manifests' / (args.run_id + '.json')).read_text())
directory = WORK / 'results' / args.run_id
directory.mkdir(parents=True, exist_ok=True)
rows = [json.loads(path.read_text()) for path in sorted(directory.glob('[0-9]*.json'))]
reference = {}
for row in rows:
    if row['backend'] == 'sequential' and row['status'] == 'ok':
        reference.setdefault(row['dataset'], row['counts'])
for row in rows:
    if row['status'] == 'ok' and row['dataset'] in reference and row['counts'] != reference[row['dataset']]:
        row['status'] = 'wrong-counts'
groups = defaultdict(list)
for row in rows:
    groups[(row['dataset'], row['backend'], row['workers'])].append(row)
medians = {key: statistics.median(r['wall_s'] for r in records if r['status'] == 'ok')
           for key, records in groups.items() if any(r['status'] == 'ok' for r in records)}
summary = []
for (dataset, backend, workers), records in sorted(groups.items()):
    good = [r for r in records if r['status'] == 'ok']
    median = medians.get((dataset, backend, workers))
    baseline = medians.get((dataset, 'sequential', 1))
    summary.append(dict(dataset=dataset, backend=backend, workers=workers,
                        completed=len(good), failed=len(records) - len(good),
                        median_s=round(median, 3) if median is not None else None,
                        speedup=round(baseline / median, 3) if baseline is not None and median else None,
                        peak_total_rss_mib=max((r['sampled_peak_total_rss_mib'] for r in good), default=None),
                        counts_checked=dataset in reference))
fields = ['dataset', 'backend', 'workers', 'completed', 'failed', 'median_s', 'speedup',
          'peak_total_rss_mib', 'counts_checked']
with (directory / 'summary.csv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=fields)
    writer.writeheader()
    writer.writerows(summary)
atomic_json(directory / 'summary.json', dict(expected=len(manifest['tasks']), recorded=len(rows),
            missing=len(manifest['tasks']) - len(rows), summary=summary,
            failures=[r for r in rows if r['status'] != 'ok']))
print('Recorded %s / %s tasks' % (len(rows), len(manifest['tasks'])))
for row in summary:
    print(row)
print('Output:', directory / 'summary.csv')
