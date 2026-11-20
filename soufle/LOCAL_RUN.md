# Ready-to-run files on this machine

Soufflé was built from this checkout with OpenMP disabled. System package
installation required a sudo password, so build tools were downloaded and
extracted locally. Prepared binaries and artifacts are under `build-local/`;
the actual CMake build directory used for compilation is `/tmp/soufle-build`.
See [PTHREADS.md](PTHREADS.md) for a normal source build and implementation details.

From `/home/vlad/nejucam/soufle`:

```bash
export BUILD_DIR="$PWD/build-local"

# Regenerate sequential/pthread C++ and compile both executables.
bash tools/cspa.sh generate

# Input graph (already provided): assign.facts and dereference.facts.
bash tools/cspa.sh sample 512
mkdir -p "$BUILD_DIR/cspa/out_seq" "$BUILD_DIR/cspa/out_pthreads"

"$BUILD_DIR/cspa/cspa_seq"   -F "$BUILD_DIR/cspa/sample_facts" -D "$BUILD_DIR/cspa/out_seq"
"$BUILD_DIR/cspa/cspa_pthreads" --execution-mode=pthreads -j 4   -F "$BUILD_DIR/cspa/sample_facts" -D "$BUILD_DIR/cspa/out_pthreads"

# Run the pthread-generated program on the calling thread instead.
"$BUILD_DIR/cspa/cspa_pthreads" --execution-mode=sequential   -F "$BUILD_DIR/cspa/sample_facts" -D "$BUILD_DIR/cspa/out_seq"

# Generate and profile a pure sequential baseline using gprof.
bash tools/cspa.sh profile
less "$BUILD_DIR/cspa/profile/gprof.txt"

# Run regression tests against this built compiler.
python3 tests/pthreads/test_codegen.py --souffle "$BUILD_DIR/src/souffle"
```

The console argument for **generation** is
`--execution-backend=sequential|pthreads`. The pthread-generated executable accepts
`--execution-mode=sequential|pthreads`, and `-j N` sets its worker count.

Generated source files are `build-local/cspa/cspa_seq.cpp` and
`build-local/cspa/cspa_pthreads.cpp`. `build-local/cspa/cspa_profile.cpp` is the
pure sequential SIZE_ONLY source compiled with `-pg`. The original unmodified
compiler and its generated source/profile are preserved under
`build-local/original/`. `build-local/validation.txt` records the completed checks.

To generate C++ directly, without the helper:

```bash
build-local/src/souffle --execution-backend=pthreads -j 4   -g build-local/cspa/cspa_pthreads.cpp ../parallel_datalog/rules/cspa.dl
```

Generated programs must use this checkout's runtime headers:

```bash
g++ -std=c++17 -O3 -g -fno-omit-frame-pointer -pthread   -DRAM_DOMAIN_SIZE=64 -Isrc/include   build-local/cspa/cspa_pthreads.cpp -o build-local/cspa/cspa_pthreads
```

For MPI generation and execution (see [MPI.md](MPI.md)):

```bash
BUILD_DIR="$PWD/build-local" bash tools/cspa.sh generate-mpi
mkdir -p build-local/cspa/out_mpi
mpirun -np 4 build-local/cspa/cspa_mpi \
  -F build-local/cspa/sample_facts -D build-local/cspa/out_mpi
python3 tests/mpi/test_codegen.py --souffle build-local/src/souffle
```

`build-local/` is ignored by Git. It contains prepared outputs, not a relocatable
CMake build tree. Rebuilding the compiler from source is described in PTHREADS.md.
