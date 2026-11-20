# Sequential and pthreads C++ generation

This checkout starts from Soufflé 2.5, commit
`5682a9f12e2668ecdd26348fe63cc508bc0fcf47`. The CSPA rules remain in the sibling
project at `../parallel_datalog/rules/cspa.dl`.

The new generator is independent of the existing OpenMP transformation and
emitter. Build with OpenMP disabled to ensure neither the compiler nor generated
executables require OpenMP. Generated programs use the original relation
implementations; pthread mode enables their concurrent locking paths.

## Build the compiler

On Ubuntu, install the required tools if necessary:

```bash
sudo apt-get install build-essential cmake ninja-build bison flex python3 binutils
```

From this repository:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSOUFFLE_DOMAIN_64BIT=ON \
  -DSOUFFLE_USE_OPENMP=OFF \
  -DSOUFFLE_USE_CURSES=OFF \
  -DSOUFFLE_USE_LIBFFI=OFF \
  -DSOUFFLE_USE_SQLITE=OFF \
  -DSOUFFLE_USE_ZLIB=OFF \
  -DSOUFFLE_ENABLE_TESTING=OFF
cmake --build build --target souffle -j2
```

The optional libraries above are unnecessary for CSPA's numeric fact files.
Use the runtime headers from this checkout when compiling generated C++.

## Generate and compile CSPA

```bash
mkdir -p build/cspa
build/src/souffle --execution-backend=sequential \
  -g build/cspa/cspa_seq.cpp ../parallel_datalog/rules/cspa.dl
build/src/souffle --execution-backend=pthreads -j 4 \
  -g build/cspa/cspa_pthreads.cpp ../parallel_datalog/rules/cspa.dl

g++ -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread \
  -DRAM_DOMAIN_SIZE=64 -Isrc/include build/cspa/cspa_seq.cpp -o build/cspa/cspa_seq
g++ -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread \
  -DRAM_DOMAIN_SIZE=64 -Isrc/include build/cspa/cspa_pthreads.cpp -o build/cspa/cspa_pthreads
```

`bash tools/cspa.sh generate` performs those steps. `BUILD_DIR`, `ARTIFACT_DIR`,
`SOUFFLE_BIN` and `CSPA_RULES` can override the defaults.

## Input facts and execution

A fact directory must contain `assign.facts` and `dereference.facts`, with two
tab-separated numeric IDs per row. An empty dereference table is permitted.
These are graph facts, not C source files. A deterministic synthetic sample is
available for profiling and exercising the parallel paths:

```bash
bash tools/cspa.sh sample 512
mkdir -p build/cspa/out_seq build/cspa/out_pthreads
build/cspa/cspa_seq -F build/cspa/sample_facts -D build/cspa/out_seq
build/cspa/cspa_pthreads --execution-mode=pthreads -j 4 \
  -F build/cspa/sample_facts -D build/cspa/out_pthreads
```

The result tables are `ValueFlow.csv`, `ValueAlias.csv`, and `MemoryAlias.csv`.
Compare them as sets of rows: physical row order is not part of correctness.

The pthread-generated executable also accepts `--execution-mode=sequential`.
This bypasses worker execution while retaining the same generated program and
thread-safe relation structures. A separately generated `cspa_seq` is the pure
serial baseline, without pthread relation-locking overhead. `-j auto` selects
the available logical CPU count in pthread mode; `-j 1` uses the calling thread.

## Profile the sequential baseline

```bash
bash tools/cspa.sh profile "$(pwd)/build/cspa/sample_facts"
less build/cspa/profile/gprof.txt
cat build/cspa/profile/time.txt
```

The script generates `cspa_profile.cpp` with `SIZE_ONLY`, compiles it with `-pg`,
runs it, and invokes `gprof`. Thus profiling does not write millions of output
rows. `sizes.txt` records relation sizes and `time.txt` records wall time and
peak resident memory. Sampling resolution can be poor on tiny inputs; use a
representative fact graph for performance conclusions. Use uninstrumented
`-O3` executables for speedup measurements, rather than comparing against `-pg`.

If Linux perf is installed and permitted, the regular serial executable can
also be sampled with `perf record -g -- build/cspa/cspa_seq -F FACTS -D OUTPUT`,
followed by `perf report`. Output and input costs are included in process timing.

## What changed

- `src/MainDriver.cpp`: `--execution-backend=sequential|pthreads`; ordinary RAM
  scans are retained instead of applying the existing OpenMP transformation.
- `src/synthesiser/Synthesiser.cpp`: eligible outer scans and indexed scans emit
  pthread range tasks, including scans that merge new facts into full relations.
- `src/include/souffle/utility/PthreadRuntime.h`: a persistent team created with
  `pthread_create`, condition-variable work dispatch, dynamic partition claiming,
  exception propagation, and a completion barrier for every submitted scan.
- `ParallelUtil.h`, `ConcurrentCache.h`, and `ConcurrentFlyweight.h`: enable
  concurrent relation/cache behavior and per-worker lanes without `_OPENMP`.
- `CompiledOptions.h`: runtime mode selection and pthread worker-count options.

Each task creates private lookup contexts. A scan finishes before the following
query, delta swap, clear, or termination check. Rule dependencies and fixpoint
rounds are unchanged. Existing B-tree partitioning supplies ranges; no OpenMP
loop or scheduler is used. Small scans may stay on the calling thread.

This first backend targets ordinary B-tree relations of up to six columns.
Queries with aggregates, early exits, custom functors, deletions, or overlapping
scan/write relations remain sequential. Outer existence scans are also kept
sequential. Provenance generation is rejected for pthread mode. Frequency
profiling conservatively disables parallel query execution; ordinary rule/round
profiling is supported. These are implementation limits, not performance claims.

## Regression checks

```bash
python3 tests/pthreads/test_codegen.py --souffle build/src/souffle
```

The test compiles generated programs without `-fopenmp`, compares all three CSPA
tables at different worker counts and runtime modes, exercises a graph large
enough to use workers, and checks the other three sibling-project rule sets.
It also checks worker synchronization, error propagation, and sequential mode.
