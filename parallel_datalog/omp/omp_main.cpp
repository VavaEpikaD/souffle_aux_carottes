// omp_main.cpp - Person A: OpenMP engine.   bin/omp -v 1|2|3 -t threads -r rules -F facts
#include "omp_backend.h"
#include "../engine/main.h"

int main(int argc, char **argv)
{
    Options o = parse_args(argc, argv);
    if (o.version < 1 || o.version > 3) DIE("omp: version must be 1, 2 or 3");
    omp_set_num_threads(o.threads);
    OmpBackend be;
    be.version = o.version;
    be.nthreads = o.threads;
    return run_engine(be, o);
}
