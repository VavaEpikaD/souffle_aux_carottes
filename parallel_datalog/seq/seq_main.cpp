// seq_main.cpp - our sequential engine: the reference for the three parallel versions (our code)
#include "../engine/host_backend.h"
#include "../engine/main.h"

int main(int argc, char **argv)
{
    Options o = parse_args(argc, argv);
    HostBackend be;
    return run_engine(be, o);
}
