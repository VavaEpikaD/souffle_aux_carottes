// mpi_main.cpp - Person B: MPI engine.   mpirun -np P bin/mpi -r rules -F facts
#include "mpi_backend.h"
#include "../engine/main.h"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    Options o = parse_args(argc, argv);
    MpiBackend be;
    MPI_Comm_rank(MPI_COMM_WORLD, &be.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &be.nprocs);
    be.version = o.version;
    run_engine(be, o);

    // how much was communicated (summed over ranks)
    uint64_t total = 0;
    MPI_Reduce(&be.bytes_sent, &total, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
    if (be.rank == 0) fprintf(stderr, "# mpi: %.1f MB sent between ranks\n", total / 1e6);
    MPI_Finalize();
    return 0;
}
