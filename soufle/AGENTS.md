# Soufflé pthread and MPI code-generation work

Last updated: 2026-11-20.

## Project and scope

This repository is `/home/vlad/nejucam/soufle`, cloned from
`https://github.com/souffle-lang/souffle` at tag **2.5**, commit
`5682a9f12e2668ecdd26348fe63cc508bc0fcf47`.

The assignment is to profile a sequential problem and parallelize it with
**pthreads, MPI and CUDA**. The work completed in this checkout covers the
sequential baseline, pthread code generation, and MPI code generation. CUDA
generation has not been implemented here. OpenMP is not a substitute for the
required pthread implementation.

The main workload is the existing CSPA Datalog program in the sibling project:
`../parallel_datalog/rules/cspa.dl`. Its inputs are `assign.facts` and
`dereference.facts`; its outputs are `ValueFlow`, `ValueAlias`, and `MemoryAlias`.
Inputs are numeric graph facts derived from code, not C/C++ source files.

The intended approach is to modify Soufflé's generator so multiple Datalog
programs can receive the new implementation. Do not confuse this with replacing
an analysis by the sibling project's handwritten relational engine. That
project's original operator architecture was restored after an earlier
misunderstanding; its OpenMP/MPI/Thrust implementations are separate work.

## What was implemented

Backend selection and runtime controls:

- Compiler: `--execution-backend=sequential|pthreads|mpi`, default `sequential`.
- Pthread-generated executable: `--execution-mode=sequential|pthreads`, default
  `pthreads`; `-j N` controls worker count and `-j auto` uses logical CPU count.
- MPI-generated executable: compile with `mpicxx`, run with `mpirun -np N`.
  MPI uses replicated integer relations and exchanges new inserts after each
  distributed query. It distributes evaluation, not relation storage.

The compiler was built with `SOUFFLE_USE_OPENMP=OFF` and 64-bit domains. The new
backend uses explicit POSIX threads and does not call the existing OpenMP
scheduler/emitter. The existing OpenMP RAM transformation is removed from this
pipeline. Specifying `-j` alone does not select pthread code generation.

A pure sequential generated executable retains upstream evaluation code. A
pthread-generated executable running in sequential mode still retains
partitioning and thread-safe relation infrastructure, so it is not identical
in overhead to the pure sequential baseline.

## Source map

- `src/MainDriver.cpp`: backend CLI selection and validation; keeps ordinary RAM
  scans rather than applying the existing OpenMP parallel transformation.
- `src/synthesiser/Synthesiser.cpp`: the shared C++ code generator.
  `canDistributeQuery()` checks query eligibility. `emitPthreadScan()` emits
  partition tasks, while `emitMpiScan()` emits tuple ownership checks. The
  ordinary `Scan` and `IndexScan` handlers select serial, pthread or MPI scans.
  The MPI `Insert` emitter buffers successful inserts, and the `Query` emitter
  synchronizes target relations before continuing.
- `src/include/souffle/utility/PthreadRuntime.h`: **new runtime file**. Implements
  `Pool`, `workerPool()`, `configure()` and `parallelFor()` using `pthread_create`,
  mutexes, condition variables, atomic task claiming and `pthread_join`.
- `src/include/souffle/utility/MpiRuntime.h`: MPI session lifecycle, deterministic
  tuple ownership, bounded broadcasts and deduplicating replica synchronization.
- `src/include/souffle/CompiledOptions.h`: runtime mode and worker-count parsing.
- `src/include/souffle/utility/ParallelUtil.h`: enables concurrent locks and
  worker lanes under `SOUFFLE_USE_PTHREADS`, independently of `_OPENMP`.
- `src/include/souffle/datastructure/ConcurrentCache.h` and
  `ConcurrentFlyweight.h`: select concurrent implementations for pthread mode.
- `tools/cspa.sh`: generate/compile the CSPA programs, create synthetic facts,
  and profile a sequential SIZE_ONLY build with gprof.
- `tests/pthreads/runtime.cpp`: worker-pool regression checks.
- `tests/pthreads/test_codegen.py`: generated-code integration checks.
- `tests/mpi/runtime.cpp` and `tests/mpi/test_codegen.py`: MPI runtime and
  complete-table comparisons at 1, 2 and 4 ranks.

The generator stays in `Synthesiser.cpp`; there is no separate pthread generator
file. The separate new file supplies the runtime called by generated code.

## Slurm deployment

The sibling `../slurm/` directory contains portable generated CSPA sources for
sequential, pthread, upstream OpenMP and MPI, upstream headers, the exact local
175,537-fact PostgreSQL subset, local benchmark references and bounded Slurm
workflows. See `../slurm/README.md`. The server deployment is `~/nejucam` on
`fep`, mounted locally at `/home/vlad/app/nejucam`. Source and Git history are
copied; machine-specific `build-local/`, sibling `bin/`, and Slurm `work/` are
excluded. Cluster executables are rebuilt on compute nodes.

`python3 slurm/submit.py all` from the deployment root submits separate build,
data and dependent benchmark arrays. Each job requests 9:30, with 480-second
compile/evaluation timeouts and a 520-second outer guard. Each benchmark task
is one backend/input/worker/repetition. Defaults use the existing `haswell`
partition and shared `app-labs.sif` image; Python runs on the host. MPI runs
inside the same image it was compiled in, on one allocated node. Larger input
subsets are prepared from `~/stadata/cspa/postgresql`. Full-dataset runs need
extra CPU/memory reservations because MPI retains complete relation replicas.

Server validation passed: build array 303123 compiled all four backends in
13–60 seconds per task; data job 303124 prepared a small subset; corrected
smoke array 303136 completed every backend with identical counts, including
four MPI ranks. The benchmark timer uses host Python because compute nodes
lack `/usr/bin/time`. See `../slurm/VALIDATION.md`. Larger benchmark arrays were
not submitted automatically.

At the user's subsequent request, the full PostgreSQL benchmark was completed
on the cluster: input job 303145, benchmark array 303146, run ID
`20261120-011934-c0a61a`. All 12 tasks (three per backend) completed successfully.
Full input: 1,209,597 assign + 3,468,946 dereference facts. Median seconds:
sequential 203.684; pthreads (4 workers) 81.070; upstream OpenMP (4 threads)
100.059; MPI (4 local ranks) 160.087. Speedups: 2.512x, 2.036x and 1.272x.
Peak sampled RSS: 9,000.5 / 9,498.2 / 9,772.9 / 38,831.6 MiB respectively;
MPI memory is summed across ranks. Counts matched in every run:
MemoryAlias 89,475,479; ValueAlias 223,793,861; ValueFlow 3,712,452.
Jobs used separate reserved Haswell nodes, 32 allocated CPUs and 48 GiB each
for the memory policy, with actual workers/ranks as above. Sources were compiled
with matched uninstrumented `-O3`, 64-bit-domain flags in the same image.
Summaries, raw JSON records, input provenance and task manifest are saved in
`../slurm/reference/full-postgresql/`. Complete tuples were not compared.
Earlier small-subset timings were on different hardware; do not attribute the
change in relative MPI performance solely to input size.

## Execution and correctness constraints

Workers dynamically claim relation partitions. Lookup contexts containing
mutable search hints are private to each chunk callback. A persistent worker
team is reused across scans and fixpoint rounds. Every submitted scan completes
before execution proceeds to the next query, merge, delta swap, clear or
termination check. Exceptions are propagated to the controlling thread after
workers finish.

The first implementation targets ordinary direct B-tree relations with up to
six columns. Queries with aggregates, early exits, deletions, guarded inserts,
auto-increment, custom functors, or overlapping scan/write relations stay
sequential. Outer existence scans also stay sequential. Provenance generation
is rejected in pthread mode. Frequency profiling conservatively disables
parallel query execution. Ordinary rule/round profiling is supported.

Preserve these eligibility checks and per-worker contexts when extending the
backend. Changing loop scheduling alone is insufficient: relation insertion,
deduplication, caches and lookup hints must remain safe for concurrent access.

## Build and prepared artifacts

See `PTHREADS.md` for normal dependency installation and full CMake commands.
See `LOCAL_RUN.md` for commands matching the prepared files on this machine.
See `MPI.md` for MPI generation, compilation, launch commands, and scope.

System installation needed a sudo password, so tools were extracted locally:

- Build tools: `/tmp/soufle-build-tools/root`.
- Actual CMake/Ninja build tree: `/tmp/soufle-build`.
- Prepared compiler: `build-local/src/souffle`.
- Generated CSPA source/executables: `build-local/cspa/`.
- Original unmodified compiler/source/profile: `build-local/original/`.
- Sequential profile report: `build-local/cspa/profile/gprof.txt`.
- Validation summary: `build-local/validation.txt`.

`build-local/` is ignored by Git and contains prepared outputs, not a relocatable
CMake build tree. `/tmp` directories may disappear; normal rebuilding is described
in `PTHREADS.md`. The copied compiler must be rebuilt and recopied after source
changes; editing a generator does not update an existing executable automatically.
Generated C++ must be compiled against this checkout's runtime headers.

From the repository root, use the prepared compiler as follows:

```bash
export BUILD_DIR="$PWD/build-local"
bash tools/cspa.sh generate
bash tools/cspa.sh sample 512
mkdir -p "$BUILD_DIR/cspa/out_seq" "$BUILD_DIR/cspa/out_pthreads"
"$BUILD_DIR/cspa/cspa_seq" \
  -F "$BUILD_DIR/cspa/sample_facts" -D "$BUILD_DIR/cspa/out_seq"
"$BUILD_DIR/cspa/cspa_pthreads" --execution-mode=pthreads -j 4 \
  -F "$BUILD_DIR/cspa/sample_facts" -D "$BUILD_DIR/cspa/out_pthreads"
bash tools/cspa.sh profile
```

Direct generation and compilation:

```bash
build-local/src/souffle --execution-backend=pthreads -j 4 \
  -g build-local/cspa/cspa_pthreads.cpp ../parallel_datalog/rules/cspa.dl

g++ -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread \
  -DRAM_DOMAIN_SIZE=64 -Isrc/include \
  build-local/cspa/cspa_pthreads.cpp -o build-local/cspa/cspa_pthreads
```

Use `--execution-backend=sequential` for pure sequential generation. To run the
pthread-generated executable without workers, use `--execution-mode=sequential`.

## MPI constraints

All ranks read identical integer facts and run the same generated program.
Eligible outer scans assign tuples using `mpi_runtime::owns()`, hashing tuple
values rather than rank-local B-tree partition boundaries. Different insertion
orders can produce different tree shapes, so partition-index ownership is unsafe.
After each distributed query, all ranks exchange successful local inserts into
each target relation in deterministic relation-name order. Replicas are complete
before the next operation, so fixpoint emptiness checks remain consistent.

Only rank 0 writes outputs. Queries outside the direct B-tree eligibility scope
run serially on every replica. Symbol/float values or intrinsic operators,
records, counters, custom functors/aggregates, choice insertion, deletion,
provenance and profiling are rejected. Keep `-j` at 1; MPI ranks are selected by
`mpirun -np N`. MPI is explicit source generation (`-g`/`--generate-many`), followed
by compilation with `mpicxx`. No MPI dependency is added to the compiler itself.
MPI_BYTE transport requires matching domain sizes and machine representations.
Every rank retains all relations; do not claim distributed-memory capacity or
MPI speedup without measurements.

## Validation completed

Run integration checks after generator/runtime changes:

```bash
python3 tests/pthreads/test_codegen.py --souffle build-local/src/souffle
```

Completed checks:

- Pure sequential CSPA source matched unmodified upstream generation
  byte-for-byte when generated with matching paths/names.
- Eight fixtures matched complete output tables: `taint_example`,
  `taint_random`, `tc_cycle`, `tc_random`, `sg_tree`, `sg_random`, `cspa_random`,
  and a larger `cspa_chain`.
- Compared pure sequential generation against pthread generation executed
  sequentially and with 1, 2 and 4 workers.
- Repeated the larger CSPA run three additional times with four workers.
- Observed five OS threads for CSPA: the main thread plus four workers.
- Tested worker synchronization, exception recovery, sequential bypass, pool
  resizing and invalid backend/runtime selections.
- Confirmed no OpenMP runtime dependency in the compiler/generated executable.
- ThreadSanitizer could not run in this environment: unexpected memory mapping.
  Do not describe these checks as a successful sanitizer audit.

MPI validation completed on 2026-11-20:

- Runtime exchange tests passed at 1, 2 and 4 ranks, both owning MPI and using
  caller-initialized MPI. Checked multiple broadcast chunks, negative/64-bit
  values, duplicate/empty contributions, and unique tuple ownership.
- Nine fixtures matched complete sequential output tables at 1, 2 and 4 ranks:
  `taint_example`, `taint_random`, `tc_cycle`, `tc_random`, `sg_tree`, `sg_random`,
  `cspa_random`, `cspa_empty`, and `cspa_chain` (512 nodes).
- Checked indexed outer scans, aggregate/outer-existence fallbacks, nullaries,
  wide relations, unsigned values and rank-0-only `.printsize` output.
- Unsupported values/options, missing inputs, and early rank exit/exception
  checks passed without leaving peers blocked in collectives.
- The documented helper generated/compiled CSPA and its four-rank run matched
  the prepared sequential sample's complete output tables.
- Re-ran the pthread integration suite successfully and reconfirmed pure
  sequential CSPA generation matches unmodified upstream byte-for-byte.
- Prepared compiler and MPI CSPA executable updated in `build-local/`.
  MPI test artifacts: `build-local/mpi-validation/`; validation summary:
  `build-local/mpi-validation.txt`.
- Subsequent PostgreSQL subset benchmark (175,537 input facts, both endpoints
  below 200,000): three interleaved runs, identical uninstrumented `-O3` /
  `SIZE_ONLY` builds. Medians: sequential 15.69 s; pthreads (4 workers) 6.68 s;
  MPI (4 local ranks, unbound) 22.09 s. MPI was 1.41x slower than sequential on
  this subset. Counts matched in all runs; complete tuples were not compared.
  Peak sampled summed MPI rank RSS: 5,276.5 MiB (1,321.2 MiB per rank peak).
  Timing includes MPI launcher startup/shutdown and all evaluation/I/O costs.
  Logs, runner and summary: `build-local/cspa/benchmark-postgresql/bench_mpi.py`,
  `mpi-comparison.json`, and `mpi-summary.txt`.
- Built-in upstream OpenMP comparison on that identical subset: generated with
  the preserved unmodified Souffle 2.5 compiler (`-j 4`, `SIZE_ONLY`), compiled
  against headers extracted from upstream HEAD with the same `-O3` flags plus
  `-fopenmp`. Three runs: 8.33, 8.22, 8.26 s; median 8.26 s, 1.90x speedup over
  the preceding sequential median. Peak RSS 1,333.5 MiB. Four OS threads were
  observed; all counts matched. OpenMP used default wait/binding policy. The
  preceding sequential/pthread/MPI results were reused for comparison.
  Artifacts: `build-local/cspa/benchmark-postgresql/openmp-summary.txt`,
  `openmp-results.json`, `bench_openmp.py`, and `openmp.cpp`/`openmp`.

The 512-node synthetic sample produced 131,328 ValueFlow rows, 262,144 ValueAlias
rows, and 4,608 MemoryAlias rows. Sequential gprof samples were dominated by
B-tree membership checks and insertion. This is a synthetic profiling example,
not evidence of representative real-program performance. Use uninstrumented
executables, comparable measurement boundaries and repeated runs for speedup
claims; do not compare pthread timing against the instrumented `-pg` baseline.
