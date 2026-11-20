# Server validation: 2026-11-20

Deployment: `fep:~/nejucam`, locally mounted at `/home/vlad/app/nejucam`.
The initial transfer archive passed SHA-256 verification before extraction.
Local source-provenance, task/dependency, resource-bound, data-filtering,
partial-summary and execution/timeout checks passed.

Slurm build array **303123** compiled all four backends on `haswell-wn29`,
using `app-labs.sif`. Batch elapsed times were 20 seconds (sequential), 60 seconds
(pthreads), 13 seconds (OpenMP), and 13 seconds (MPI). Data job **303124**
completed in four seconds and prepared 11,230 assign + 32,057 dereference facts
with both endpoints below 50,000.

The first smoke array exposed the compute nodes' missing `/usr/bin/time`;
the runner now uses Python's monotonic clock and a memory-sampling thread.
Corrected smoke array **303136** completed all four tasks with exit code 0,
including four MPI ranks. All backends produced identical counts:

```text
MemoryAlias 58062
ValueAlias 124617
ValueFlow 32704
```

This is a workflow smoke test. Observed process/thread counts confirmed serial,
pthread, OpenMP and four-rank MPI execution. Run ID:
`20261120-011320-ce3929`. Full records/logs remain in `slurm/work/`; summaries
are copied to `slurm/reference/slurm-smoke/`.

The default 200,000-endpoint subset is shipped unchanged from the local
175,537-fact benchmark. Larger benchmarks were not submitted automatically.
`python3 slurm/submit.py all` launches the normal three-repetition comparison.

## Full PostgreSQL benchmark

At the user's subsequent request, input job **303145** and benchmark array
**303146** ran the full 4,678,543-fact PostgreSQL case. All twelve tasks completed
successfully, three per backend, with matching counts and no timeouts. Run ID:
`20261120-011934-c0a61a`.

Median seconds: sequential **203.684**, pthreads (4) **81.070**, OpenMP (4)
**100.059**, MPI (4 local ranks) **160.087**. Corresponding speedups are 2.512x,
2.036x and 1.272x. Peak sampled memory was 8.79, 9.28, 9.54 and 37.92 GiB;
MPI memory is summed over ranks. Each task requested an exclusive Haswell node,
32 allocated CPUs and 48 GiB, while actual workers/ranks remained as above.
The cluster's two-running-jobs-per-user policy limited concurrency.

Raw records, input/source/build provenance, CSV/JSON summaries and a text report
are preserved in `slurm/reference/full-postgresql/`. Timings include loading,
container/process startup and evaluation, with MPI communication and launcher
costs. Comparison with the earlier small local test changes both hardware and
input size, so it does not isolate input-size effects.
