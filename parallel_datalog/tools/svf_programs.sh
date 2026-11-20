#!/bin/bash
# svf_programs.sh - facts from real C programs with an existing extractor (SVF) (our code)
#
# usage: bash tools/svf_programs.sh          (needs docker + clang-18, see README)
#
# For bzip2 1.0.8, Lua 5.4.7 and the SQLite amalgamation:
#   1. clang-18 -emit-llvm  -> LLVM bitcode of the whole program (llvm-link)
#   2. SVF (Docker image svftools/svf) `wpa -ander -print-pag` -> pointer assignment graph
#   3. tools/svf_to_facts.py -> inputs/cspa_<prog>/ (assign, dereference) and
#      inputs/taint_<prog>/ (Flow + random Source/Sink/Sanitized, like the other taint inputs)
cd "$(dirname "$0")/.."
TOOLS=$PWD/tools
DATA=${STADATA:-$HOME/stadata}
SRC=$DATA/src
mkdir -p "$SRC"
cd "$SRC" || exit 1
CC="clang-18 -O0 -g0 -Xclang -disable-O0-optnone -emit-llvm -w"

[ -d bzip2-1.0.8 ] || curl -sSL https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz | tar xz
[ -d lua-5.4.7 ] || curl -sSL https://www.lua.org/ftp/lua-5.4.7.tar.gz | tar xz
if [ ! -d sqlite ]; then
    curl -sSL -o sqlite.zip https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip &&
        unzip -q sqlite.zip && mv sqlite-amalgamation-* sqlite && rm sqlite.zip
fi

build() {   # build <name> <dir> <c files...>
    local name=$1 dir=$2
    shift 2
    (cd "$dir" && for f in "$@"; do $CC -c "$f" -o "${f%.c}.bc"; done &&
        llvm-link-18 $(for f in "$@"; do echo "${f%.c}.bc"; done) -o "$SRC/$name.bc")
}
[ -f bzip2.bc ] || build bzip2 bzip2-1.0.8 blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib.c bzip2.c
[ -f lua.bc ] || build lua lua-5.4.7/src $(cd lua-5.4.7/src && ls *.c | grep -v luac.c)
[ -f sqlite.bc ] || build sqlite sqlite sqlite3.c shell.c

for p in bzip2 lua sqlite; do
    [ -f "$SRC/$p.pag" ] && continue
    echo "SVF on $p ..."
    docker run --rm -v "$SRC":/w svftools/svf:latest bash -c \
        "cd /w && timeout 1800 wpa -ander -print-pag $p.bc > $p.pag 2> $p.wpa.err"
done
for p in bzip2 lua sqlite; do
    [ -s "$SRC/$p.pag" ] || { echo "no PAG for $p"; continue; }
    python3 "$TOOLS/svf_to_facts.py" "$SRC/$p.pag" "$DATA/inputs/cspa_$p" "$DATA/inputs/taint_$p"
    python3 "$TOOLS/make_taint_facts.py" "$DATA/inputs/taint_$p/flow_edges.tsv" "$DATA/inputs/taint_$p"
done
