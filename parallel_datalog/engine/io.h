// io.h - read Soufflé-style .facts files, write results (our code)
#ifndef IO_H
#define IO_H

#include <map>
#include "rel.h"

// all input relations of one run, on the host
struct Facts {
    std::string dir;
    std::map<std::string, std::vector<tup>> bin;          // binary relations
    std::map<std::string, std::vector<uint32_t>> un;      // unary relations

    // read <dir>/<name>.facts (tab separated numbers)
    const std::vector<tup> &binary(const std::string &name);
    const std::vector<uint32_t> &unary(const std::string &name);
};

// a result relation, gathered on the host (or only its size + hash for big runs)
struct Result {
    std::string name;
    uint64_t size = 0;
    uint64_t hash = 0;
};

std::vector<tup> flipped(const std::vector<tup> &v);

// write a relation as <dir>/<name>.csv, sorted, tab separated (same as Soufflé's output)
void write_csv(const std::string &dir, const std::string &name, std::vector<tup> v);

#endif
