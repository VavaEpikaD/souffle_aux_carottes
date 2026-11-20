#!/bin/bash
# Run on the development machine after building the modified generator.
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SOUFFLE_BIN=${SOUFFLE_BIN:-$ROOT/soufle/build-local/src/souffle}
UPSTREAM_SOUFFLE_BIN=${UPSTREAM_SOUFFLE_BIN:-$ROOT/soufle/build-local/original/souffle}
RULES=${CSPA_RULES:-$ROOT/parallel_datalog/rules/cspa.dl}
mkdir -p "$ROOT/slurm/generated" "$ROOT/slurm/upstream"
for backend in sequential pthreads mpi; do
    "$SOUFFLE_BIN" --execution-backend="$backend" -M SIZE_ONLY \
        -g "$ROOT/slurm/generated/$backend.cpp" "$RULES"
done
"$UPSTREAM_SOUFFLE_BIN" -j 4 -M SIZE_ONLY -g "$ROOT/slurm/generated/openmp.cpp" "$RULES"
git -C "$ROOT/soufle" archive 5682a9f12e2668ecdd26348fe63cc508bc0fcf47 src/include |
    tar -x -C "$ROOT/slurm/upstream"
python3 - "$ROOT" <<'PY'
from pathlib import Path
import hashlib, json, sys
root=Path(sys.argv[1])
files={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()
       for p in (root/'slurm/generated').glob('*.cpp')}
(root/'slurm/generated/manifest.json').write_text(json.dumps(dict(
    souffle_commit='5682a9f12e2668ecdd26348fe63cc508bc0fcf47', domain_bits=64,
    size_only=True, sources=files),indent=2)+'\n')
PY
