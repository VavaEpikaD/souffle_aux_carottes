# CSPA benchmarks on the Slurm server

The deployment lives at `~/nejucam` on `fep` (locally mounted at
`/home/vlad/app/nejucam`). It contains the modified Soufflé source, the sibling
`parallel_datalog` project, portable generated C++ for four backends, pristine
upstream runtime headers, and the exact PostgreSQL subset previously tested
locally: 42,217 assign + 133,320 dereference facts.

Run submission commands **on the server**, from the project root:

```bash
ssh fep
cd ~/nejucam
python3 slurm/submit.py all
```

This submits a four-task build array, a data-preparation job, and a benchmark
array dependent on both. The default benchmark compares sequential, pthreads,
built-in Soufflé OpenMP and MPI on the original 175,537-fact subset, with four
workers/ranks and three repetitions. Sequential runs use one active CPU.
Each result is a separate array task. Tasks run one at a time by default to
reduce interference. `--concurrency N` changes that limit, including for builds.
For concurrent benchmark jobs, `--exclusive` requests separate reserved nodes.
The server's per-user limit currently permits two running jobs, even if a
larger array concurrency is requested.

## Ten-minute job limit

Every batch script requests **9 minutes 30 seconds**. Each C++ compilation and
benchmark execution has a **480-second timeout**, plus a 15-second termination
grace period; an outer 520-second guard leaves time before Slurm terminates
the allocation. Generation, compilation and evaluation do not share one job.
Each timed-out run keeps its stdout/stderr and JSON record; other array tasks
can still complete. A timed-out build can be resubmitted for that backend.
An interrupted individual C++ compilation restarts that compilation.

The entire suite can take more than ten minutes across multiple jobs. A single
evaluation that takes longer than eight minutes is recorded as a timeout; it
does not resume in a later job. Choose smaller inputs or more workers for it.

## Separate steps and larger tests

```bash
# Build only: select any subset of the four backends.
python3 slurm/submit.py build
python3 slurm/submit.py build --backends mpi

# Stream server facts into subsets. Limits apply to BOTH numeric endpoints.
python3 slurm/submit.py data --sizes 50000,200000,500000,1000000,full

# Once binaries and data exist, submit a scaling study.
python3 slurm/submit.py bench --sizes 200000 --workers 1,2,4,8 --reps 3

# Or build, prepare and benchmark with dependencies in one command.
python3 slurm/submit.py all --sizes 200000 --workers 1,2,4,8

# A larger subset with more memory reserved for replicated MPI relations.
python3 slurm/submit.py all --sizes 500000 --workers 1,2,4 --cpus 32 --mem 48G

# Full PostgreSQL needs more memory. Reserve more CPUs to satisfy this
# partition's memory-per-CPU policy, while keeping actual workers at four.
python3 slurm/submit.py all --sizes full --workers 4 --cpus 32 --mem 48G --reps 1

# Use successful existing job IDs as dependencies, or inspect without submitting.
python3 slurm/submit.py bench --after 12345:12346
python3 slurm/submit.py all --dry-run
```

Full facts default to `~/stadata/cspa/postgresql`; override with `--facts DIR`.
Numeric limits are induced graph subsets, not percentages or numbers of rows.
The shipped `200000` subset is used unchanged. Other sizes are streamed from
the server's original facts. `full` uses the original directory without copying
it. Checksums and input counts are saved with each result.

Builds use identical `-std=c++17 -O3 -DNDEBUG -pthread -DRAM_DOMAIN_SIZE=64`
flags; OpenMP additionally uses `-fopenmp`, while MPI uses `mpicxx`. There is no
`-march=native` or profiler instrumentation. Each generated program prints only
relation sizes. Change `SIF` to choose a different image, or set
`USE_CONTAINER=0` to use native compilers/runtimes:

```bash
SIF=/path/to/image.sif python3 slurm/submit.py all
USE_CONTAINER=0 python3 slurm/submit.py all
```

The default image is
`/export/home/acs/prof/stefan_dan.ciocirlan/TMP/DO_NOT_DELETE_IMGS/app-labs.sif`.
Python, Slurm and GNU timeout run on the host. Python measures elapsed time
and samples memory without requiring `/usr/bin/time`. The image supplies
GCC 13 and Open MPI 4.1.6. Compilers and MPI launcher/executable all use that
same image. **MPI jobs currently stay on one node** and launch local ranks
inside the allocation; this avoids depending on host/container MPI or PMIx
compatibility. MPI memory is replicated per rank. Large rank counts on the
full dataset may exceed a node's memory allocation.

Jobs default to `haswell`, with 1,700 MiB allocated per CPU. The observed
partition limit is 1,750 MiB per CPU; `--cpus` and `--mem` let you reserve extra
memory without increasing the program's worker count. `--partition` selects
another partition. Runtime OpenMP wait policy defaults to the runtime's
default, as in the local measurements; `--omp-wait passive` is also available.

## Logs, correctness and summaries

Submission prints its job IDs and run ID. Inspect jobs and summarize results:

```bash
squeue -u "$USER"
sacct -j JOB_ID --format=JobID,JobName,State,Elapsed,MaxRSS
python3 slurm/summarize.py RUN_ID
```

- Logs: `slurm/work/logs/`.
- Compiler outputs and build metadata: `slurm/work/bin/`.
- Input manifests and generated subsets: `slurm/work/data/`.
- Immutable task manifests: `slurm/work/manifests/`.
- Raw results, stdout/stderr, CSV/JSON summaries: `slurm/work/results/RUN_ID/`.

One timing includes process/container startup, input loading, evaluation and
printing counts; MPI also includes launcher startup, communication and shutdown.
Build and queue time are excluded. Result records include node, compiler flags,
binary/source/input hashes, worker count, observed processes/threads and sampled
memory. Summed rank RSS is sampled every 100 ms and can count shared pages more
than once. Median and speedup use successful runs; failures and missing records
remain visible. Counts are checked against the known local result for `200000`
and against sequential results when summarizing other sizes. Matching counts
alone do not prove complete tuple equality.

## Changing rules or generator code

Source for all generator changes is under `soufle/`. The supplied C++ files
are already generated, so the cluster workflow does not need to rebuild the
Soufflé compiler within a ten-minute allocation. To change rules or generator
code, rebuild the compiler on a suitable development machine following
`soufle/PTHREADS.md`, then run:

```bash
bash slurm/prepare_sources.sh
```

`SOUFFLE_BIN`, `UPSTREAM_SOUFFLE_BIN` and `CSPA_RULES` can override the prepared
compiler/rule paths. The OpenMP source must come from the unmodified upstream
compiler. Transfer the regenerated sources/runtime headers to the server, then
resubmit the build array. Old local executables/CMake build trees are excluded
from deployment; all cluster executables are compiled on compute nodes.
