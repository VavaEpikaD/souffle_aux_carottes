#!/usr/bin/env bash
# Build inspectable CSPA programs and a sequential gprof baseline.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-$ROOT/build}
ARTIFACT_DIR=${ARTIFACT_DIR:-$BUILD_DIR/cspa}
SOUFFLE_BIN=${SOUFFLE_BIN:-$BUILD_DIR/src/souffle}
CSPA_RULES=${CSPA_RULES:-$ROOT/../parallel_datalog/rules/cspa.dl}
CSPA_RULES=$(realpath "$CSPA_RULES")
CXX=${CXX:-g++}
MPICXX=${MPICXX:-mpicxx}
mkdir -p "$ARTIFACT_DIR"
ARTIFACT_DIR=$(cd "$ARTIFACT_DIR" && pwd)

compile() {
    "$CXX" -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread \
        -DRAM_DOMAIN_SIZE=64 -I"$ROOT/src/include" "$1" -o "$2"
}

case ${1:-generate} in
    generate-mpi)
        "$SOUFFLE_BIN" --execution-backend=mpi -g "$ARTIFACT_DIR/cspa_mpi.cpp" "$CSPA_RULES"
        "$MPICXX" -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread \
            -DRAM_DOMAIN_SIZE=64 -I"$ROOT/src/include" \
            "$ARTIFACT_DIR/cspa_mpi.cpp" -o "$ARTIFACT_DIR/cspa_mpi"
        echo "Generated MPI C++ and executable: $ARTIFACT_DIR"
        ;;
    generate)
        "$SOUFFLE_BIN" --execution-backend=sequential -g "$ARTIFACT_DIR/cspa_seq.cpp" "$CSPA_RULES"
        "$SOUFFLE_BIN" --execution-backend=pthreads -j 4 -g "$ARTIFACT_DIR/cspa_pthreads.cpp" "$CSPA_RULES"
        compile "$ARTIFACT_DIR/cspa_seq.cpp" "$ARTIFACT_DIR/cspa_seq"
        compile "$ARTIFACT_DIR/cspa_pthreads.cpp" "$ARTIFACT_DIR/cspa_pthreads"
        echo "Generated C++ and executables: $ARTIFACT_DIR"
        ;;
    sample)
        python3 - "$ARTIFACT_DIR/sample_facts" "${2:-512}" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1]); count = int(sys.argv[2])
if count < 2:
    raise SystemExit('sample size must be at least 2')
out.mkdir(parents=True, exist_ok=True)
(out / 'assign.facts').write_text(''.join(f'{i}\t{i+1}\n' for i in range(count - 1)))
(out / 'dereference.facts').write_text(''.join(f'{i}\t{count+i}\n' for i in range(0, count, 8)))
print(f'{count} value nodes; sample facts in {out}')
PY
        ;;
    profile)
        FACTS=${2:-$ARTIFACT_DIR/sample_facts}
        FACTS=$(cd "$FACTS" && pwd)
        mkdir -p "$ARTIFACT_DIR/profile"
        # Print result counts instead of writing potentially huge result tables.
        "$SOUFFLE_BIN" --execution-backend=sequential -M SIZE_ONLY \
            -g "$ARTIFACT_DIR/cspa_profile.cpp" "$CSPA_RULES"
        "$CXX" -std=c++17 -O2 -g -pg -fno-omit-frame-pointer -pthread \
            -DRAM_DOMAIN_SIZE=64 -I"$ROOT/src/include" \
            "$ARTIFACT_DIR/cspa_profile.cpp" -o "$ARTIFACT_DIR/cspa_profile"
        (cd "$ARTIFACT_DIR/profile"
         /usr/bin/time -v "$ARTIFACT_DIR/cspa_profile" -F "$FACTS" > sizes.txt 2> time.txt
         gprof -b "$ARTIFACT_DIR/cspa_profile" gmon.out > gprof.txt)
        echo "Profile: $ARTIFACT_DIR/profile/gprof.txt"
        ;;
    *) echo "usage: bash tools/cspa.sh generate|generate-mpi|sample [nodes]|profile [fact-directory]" >&2; exit 1 ;;
esac
