// main.h - the same main() for every back-end (our code)
//
// usage: <engine> -r tc|sg|taint|cspa -F factdir [-D outdir] [-t threads] [-v version] [-n name]
//
// Prints one CSV line (only rank 0 for MPI):
//   engine,version,threads,rules,input,load_s,solve_s,iters,join_s,sort_s,diff_s,comm_s,rss_MB,results
// results = "Rel=size:hash|Rel=size:hash..." (hash = order-independent, see rel.h), so any
// two engines (and Soufflé, via tools/relhash) can be compared exactly.
#ifndef MAIN_H
#define MAIN_H

#include <unistd.h>
#include <sys/resource.h>
#include <cstring>
#include "programs.h"

struct Options {
    std::string rules, facts, out, name;
    int threads = 1, version = 1;
};

inline Options parse_args(int argc, char **argv)
{
    Options o;
    int c;
    while ((c = getopt(argc, argv, "r:F:D:t:v:n:")) != -1) {
        switch (c) {
        case 'r': o.rules = optarg; break;
        case 'F': o.facts = optarg; break;
        case 'D': o.out = optarg; break;
        case 't': o.threads = atoi(optarg); break;
        case 'v': o.version = atoi(optarg); break;
        case 'n': o.name = optarg; break;
        default:
            fprintf(stderr, "usage: %s -r tc|sg|taint|cspa -F factdir [-D outdir] [-t threads] [-v version]\n", argv[0]);
            exit(1);
        }
    }
    if (o.rules.empty() || o.facts.empty()) DIE("need -r and -F");
    if (o.name.empty()) {
        std::string f = o.facts;
        while (!f.empty() && f.back() == '/') f.pop_back();
        o.name = f.substr(f.find_last_of('/') + 1);
    }
    return o;
}

// which input relations each rule set reads (read before the timer starts)
inline void preload(Facts &F, const std::string &rules)
{
    if (rules == "tc" || rules == "sg") F.binary("edge");
    else if (rules == "taint") { F.binary("Flow"); F.binary("Source"); F.binary("Sink"); F.unary("Sanitized"); }
    else if (rules == "cspa") { F.binary("assign"); F.binary("dereference"); }
    else DIE("unknown rule set %s", rules.c_str());
}

template <class B>
int run_engine(B &be, const Options &o)
{
    Facts F;
    F.dir = o.facts;
    double t0 = now_sec();
    preload(F, o.rules);
    double load_s = now_sec() - t0;

    be.outdir = o.out;
    std::vector<Result> res;
    be.barrier();
    t0 = now_sec();
    if (o.rules == "tc") prog_tc(be, F, res);
    else if (o.rules == "sg") prog_sg(be, F, res);
    else if (o.rules == "taint") prog_taint(be, F, res);
    else prog_cspa(be, F, res);
    be.barrier();
    double solve_s = now_sec() - t0;

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    if (be.is_root()) {
        std::string r;
        for (auto &x : res) {
            char buf[128];
            snprintf(buf, sizeof buf, "%s%s=%llu:%016llx", r.empty() ? "" : "|", x.name.c_str(),
                     (unsigned long long)x.size, (unsigned long long)x.hash);
            r += buf;
        }
        printf("%s,v%d,%d,%s,%s,%.3f,%.3f,%ld,%.3f,%.3f,%.3f,%.3f,%.0f,%s\n",
               be.name(), o.version, be.workers(), o.rules.c_str(), o.name.c_str(),
               load_s, solve_s, be.prof.iters, be.prof.join, be.prof.sort, be.prof.diff, be.prof.comm,
               ru.ru_maxrss / 1024.0, r.c_str());
        fflush(stdout);
    }
    return 0;
}

#endif
