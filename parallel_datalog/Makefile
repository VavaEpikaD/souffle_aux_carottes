# Parallel Datalog engine for program analysis - build (run inside WSL / Linux)
#
#   make            Soufflé baselines + seq + omp + mpi + thrust (OpenMP back-end of Thrust)
#   make gpu        bin/thrust_gpu (needs nvcc + an NVIDIA GPU: Google Colab, see scripts/colab.md)
#   make test       every engine vs hand-written results and vs Soufflé on small inputs
#   make clean

CXX      = g++
MPICXX   = mpicxx
NVCC     = nvcc
CXXFLAGS = -O3 -march=native -std=c++17 -Wall -Wextra -Wno-unused-parameter
RULES    = tc sg taint cspa

ENGINE_H = $(wildcard engine/*.h)
ENGINE_C = engine/io.cpp

SOUFFLE_BIN = $(foreach r,$(RULES),bin/souffle_$(r) bin/souffle_$(r)_seq bin/souffle_$(r)_par)
ENGINES     = bin/seq bin/omp bin/thrust_omp bin/relhash
ifneq ($(shell command -v $(MPICXX) 2> /dev/null),)
ENGINES    += bin/mpi
endif

all: $(SOUFFLE_BIN) $(ENGINES)

# ---- Soufflé baseline: the C++ Soufflé generates for our rules (kept in bin/*.cpp) ----
# IMPORTANT: Soufflé only generates parallel code (pfor / PARALLEL_START) when -j is given
# at COMPILE time. A program compiled without it ignores `-j 16` at run time and runs serially.
#   souffle_<rules>      parallel code, writes the relations as CSV (tests, small inputs)
#   souffle_<rules>_seq  serial code, prints only sizes   = the sequential baseline (T1)
#   souffle_<rules>_par  parallel code, prints only sizes = Soufflé's OpenMP baseline, run with -j N
bin/souffle_%: rules/%.dl
	@mkdir -p bin
	souffle -w -j 16 -o $@ $<

bin/souffle_%_seq: rules/%.dl
	@mkdir -p bin
	souffle -w -M "SIZE_ONLY" -o $@ $<

bin/souffle_%_par: rules/%.dl
	@mkdir -p bin
	souffle -w -j 16 -M "SIZE_ONLY" -o $@ $<

# ---- our engines ----
bin/seq: seq/seq_main.cpp $(ENGINE_C) $(ENGINE_H)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) seq/seq_main.cpp $(ENGINE_C) -o $@

bin/omp: omp/omp_main.cpp omp/omp_backend.h $(ENGINE_C) $(ENGINE_H)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -fopenmp omp/omp_main.cpp $(ENGINE_C) -o $@

bin/mpi: mpi/mpi_main.cpp mpi/mpi_backend.h $(ENGINE_C) $(ENGINE_H)
	@mkdir -p bin
	$(MPICXX) $(CXXFLAGS) mpi/mpi_main.cpp $(ENGINE_C) -o $@

# Thrust code compiled by g++ with Thrust's OpenMP "device": runs here, no GPU needed
bin/thrust_omp: cuda/thrust_main.cu cuda/thrust_backend.h $(ENGINE_C) $(ENGINE_H)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -fopenmp -x c++ -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP \
		cuda/thrust_main.cu -x none $(ENGINE_C) -o $@

# the same file compiled by nvcc for a real GPU
gpu: bin/thrust_gpu
bin/thrust_gpu: cuda/thrust_main.cu cuda/thrust_backend.h $(ENGINE_C) $(ENGINE_H)
	@mkdir -p bin
	$(NVCC) -O3 -std=c++17 $(NVFLAGS) cuda/thrust_main.cu $(ENGINE_C) -o $@

bin/relhash: tools/relhash.cpp engine/rel.h
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) tools/relhash.cpp -o $@

test: all
	bash tests/run_tests.sh

clean:
	rm -rf bin

.PHONY: all gpu test clean
.PRECIOUS: bin/souffle_%
