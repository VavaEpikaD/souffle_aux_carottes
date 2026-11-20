// relhash.cpp - size + order-independent hash of a relation written by Soufflé (our code)
//
// usage: relhash OUTDIR Rel1 [Rel2 ...]     reads OUTDIR/<Rel>.csv (2 numeric columns)
// prints  Rel1=size:hash|Rel2=size:hash   in the same format as our engines, so a run of
// our engine can be compared with Soufflé by comparing one string.
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "../engine/rel.h"

int main(int argc, char **argv)
{
    if (argc < 3) DIE("usage: relhash OUTDIR Rel...");
    std::string out;
    for (int i = 2; i < argc; i++) {
        std::string path = std::string(argv[1]) + "/" + argv[i] + ".csv";
        FILE *f = fopen(path.c_str(), "r");
        if (!f) DIE("cannot open %s", path.c_str());
        std::vector<tup> v;
        unsigned a, b;
        while (fscanf(f, "%u %u", &a, &b) == 2) v.push_back(make_tup(a, b));
        fclose(f);
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        uint64_t h = 0;
        for (tup t : v) h += mix64(t);
        char buf[160];
        snprintf(buf, sizeof buf, "%s%s=%zu:%016llx", out.empty() ? "" : "|", argv[i], v.size(),
                 (unsigned long long)h);
        out += buf;
    }
    printf("%s\n", out.c_str());
    return 0;
}
