# MPI C++ generation

Select `--execution-backend=mpi` to generate MPI evaluation code for integer
Datalog programs, including the sibling project's CSPA rules. The compiler
itself does not link MPI. Compile its generated source with `mpicxx`, and choose
the process count with `mpirun -np N`. This backend uses MPI processes directly.

## Build and run

Build the compiler as described in [PTHREADS.md](PTHREADS.md), with OpenMP off.
Install an MPI development package and launcher if necessary, for example
`libopenmpi-dev` and `openmpi-bin` on Ubuntu. From this repository:

```bash
mkdir -p build-local/cspa/out_mpi
build-local/src/souffle --execution-backend=mpi \
  -g build-local/cspa/cspa_mpi.cpp ../parallel_datalog/rules/cspa.dl
mpicxx -std=c++17 -O3 -pthread -DRAM_DOMAIN_SIZE=64 -Isrc/include \
  build-local/cspa/cspa_mpi.cpp -o build-local/cspa/cspa_mpi
mpirun -np 4 build-local/cspa/cspa_mpi \
  -F build-local/cspa/sample_facts -D build-local/cspa/out_mpi
```

Alternatively, `BUILD_DIR="$PWD/build-local" bash tools/cspa.sh generate-mpi`
generates and compiles CSPA using `MPICXX` (default `mpicxx`). The existing
`sample` command can create numeric sample facts. For counts instead of CSVs,
pass `-M SIZE_ONLY` when generating CSPA and compile normally without `-pg`.

Only rank 0 writes result tables, `.printsize` output, `printAll`, or dumps.
Every rank must be able to read identical input facts. For multiple machines,
provide the executable and facts on each machine or on a shared filesystem, and
use compatible domain sizes and machine representations. The bounded tuple
exchange uses `MPI_BYTE`, so heterogeneous endianness is not supported.

`-j` is not the MPI process count: leave it at 1 and use `mpirun -np N`.
Compiler `-o`/automatic compilation is rejected for MPI; use `-g` or
`--generate-many` and compile the resulting C++ with the MPI wrapper.

## Evaluation and synchronization

Each rank holds a complete relation replica and loads the same inputs.
Eligible outermost `Scan` and `IndexScan` operations assign tuples to ranks
using a deterministic hash of their numeric values. Every rank traverses the
outer scan, but only the owner executes its nested joins, filters and inserts.
Ownership does not depend on B-tree structure or insertion order.

Successful local insertions are buffered separately for each target relation.
After the query, each rank broadcasts its buffer in bounded chunks; every
other rank inserts those tuples using the existing deduplicating relations.
Only new local inserts are exchanged, not the complete existing relations.
All ranks complete these collectives before the next query, merge, delta swap,
clear or termination check. Thus the existing semi-naive fixpoint evaluation
sees identical relation contents everywhere at query boundaries. A rank with
no work still participates in the exchanges.

This implementation distributes evaluation, not relation storage. Memory per
rank includes a complete database and the current query's local insert buffer.
It may use more total memory than sequential evaluation. Full outer scans,
replication and per-query communication can outweigh saved join work; speedup
depends on the workload. No MPI performance claim is implied by correctness
tests.

## Supported scope

The backend accepts signed/unsigned integer relation columns. Distributed
queries target ordinary direct B-trees with 1–6 columns. Aggregates, outer
existence scans, early-exit queries, nullary/wide/specialized relations, and
scan/write overlaps retain serial evaluation independently on every replica.
Programs using symbol/float values or intrinsic operators, records, counters,
custom functors or custom aggregates, choice insertion, deletion, provenance, or profiling are
rejected. MPI does not enable the pthread or OpenMP runtime paths.

Generated `main` initializes and finalizes MPI. Exceptions and existing I/O
`exit(1)` paths abort peer ranks rather than leaving them in collectives.
MPI job-launcher failure handling still applies to external process failures.
For embedding, construct `souffle::mpi_runtime::Session` on every rank before
using the generated program, then call it collectively with identical data.
An already-initialized MPI runtime stays owned by the caller. The runtime uses
`MPI_COMM_WORLD`; independent subcommunicator executions are not supported.

## Verification

```bash
python3 tests/mpi/test_codegen.py --souffle build-local/src/souffle
python3 tests/pthreads/test_codegen.py --souffle build-local/src/souffle
```

The MPI suite compares complete tables for CSPA, transitive closure, same
generation and taint fixtures at 1, 2 and 4 ranks. It includes empty CSPA input,
a 512-node recursive chain, fallback queries, unsigned values, and rejection
of unsupported input/options. Runtime checks cover chunked exchange, 64-bit
and negative values, duplicate/empty contributions, MPI ownership/finalization,
and early rank failures. Open MPI needs permission to create local sockets.
