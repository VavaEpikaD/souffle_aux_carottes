// thrust_main.cu - Person C: Thrust engine (GPU with nvcc, OpenMP with g++).
//   bin/thrust_gpu -r rules -F facts          bin/thrust_omp -t threads -r rules -F facts
#include "thrust_backend.h"
#include "../engine/main.h"
#ifndef __CUDACC__
#include <omp.h>
#endif

int main(int argc, char **argv)
{
    Options o = parse_args(argc, argv);
    ThrustBackend be;
#ifndef __CUDACC__
    omp_set_num_threads(o.threads);
    be.nthreads = o.threads;
#else
    cudaFree(0);    // create the CUDA context before the timer starts
#endif
    return run_engine(be, o);
}
