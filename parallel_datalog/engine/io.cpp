#include <algorithm>
#include <cstring>
#include "io.h"

// fast enough reader: whole file into memory, parse numbers by hand
static std::string read_file(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) DIE("cannot open %s", path.c_str());
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string s(n, '\0');
    if (n && fread(&s[0], 1, n, f) != (size_t)n) DIE("cannot read %s", path.c_str());
    fclose(f);
    return s;
}

static void parse_numbers(const std::string &s, int per_line, std::vector<uint32_t> &out)
{
    const char *p = s.c_str(), *end = p + s.size();
    while (p < end) {
        uint32_t vals[2] = {0, 0};
        int got = 0;
        // one line
        while (p < end && *p != '\n') {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
            if (p >= end || *p == '\n') break;
            if (*p < '0' || *p > '9') DIE("non-numeric fact in input (only numbers are supported)");
            uint64_t v = 0;
            while (p < end && *p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
            if (got < 2) vals[got] = (uint32_t)v;
            got++;
        }
        if (p < end) p++;   // newline
        if (got == 0) continue;
        if (got < per_line) DIE("line with %d columns, expected %d", got, per_line);
        for (int i = 0; i < per_line; i++) out.push_back(vals[i]);
    }
}

const std::vector<tup> &Facts::binary(const std::string &name)
{
    auto it = bin.find(name);
    if (it != bin.end()) return it->second;
    std::vector<uint32_t> nums;
    parse_numbers(read_file(dir + "/" + name + ".facts"), 2, nums);
    std::vector<tup> v(nums.size() / 2);
    for (size_t i = 0; i < v.size(); i++) v[i] = make_tup(nums[2 * i], nums[2 * i + 1]);
    return bin[name] = std::move(v);
}

const std::vector<uint32_t> &Facts::unary(const std::string &name)
{
    auto it = un.find(name);
    if (it != un.end()) return it->second;
    std::vector<uint32_t> nums;
    parse_numbers(read_file(dir + "/" + name + ".facts"), 1, nums);
    return un[name] = std::move(nums);
}

std::vector<tup> flipped(const std::vector<tup> &v)
{
    std::vector<tup> r(v.size());
    for (size_t i = 0; i < v.size(); i++) r[i] = flip_tup(v[i]);
    return r;
}

void write_csv(const std::string &dir, const std::string &name, std::vector<tup> v)
{
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    std::string path = dir + "/" + name + ".csv";
    FILE *f = fopen(path.c_str(), "w");
    if (!f) DIE("cannot write %s", path.c_str());
    for (tup t : v) fprintf(f, "%u\t%u\n", col0(t), col1(t));
    fclose(f);
}
