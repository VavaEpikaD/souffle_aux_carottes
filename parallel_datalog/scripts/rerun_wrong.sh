#!/bin/bash
# rerun_wrong.sh - drop benchmark rows marked WRONG (runs made with the buggy merge before the fix)
# and measure those configurations again with the fixed binary (our code)
cd "$(dirname "$0")/.."
python3 - <<'PY'
import csv
p = 'results/bench/bench.csv'
rows = list(csv.DictReader(open(p)))
bad = {(r['input'], r['engine'], r['threads']) for r in rows if r['status'] == 'WRONG'}
keep = [r for r in rows if (r['input'], r['engine'], r['threads']) not in bad]
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=list(rows[0]))
w.writeheader()
w.writerows(keep)
print('removed', sorted(bad))
PY
OMP_WAIT_POLICY=passive python3 scripts/bench.py --threads 16 --engines omp1,omp2,omp3 \
  --inputs cspa_bzip2,cspa_lua,cspa_linux,tc_SF.cedge,taint_p2p-Gnutella04,cspa_httpd,sg_ego-Facebook
echo rerun_done
