#!/bin/bash
# download_data.sh - real-program facts and benchmark graphs (our code)
#
# usage: bash tools/download_data.sh [DATA_DIR]       (default $STADATA or ~/stadata)
#
#   cspa/<httpd|linux|postgresql>/{assign,dereference}.facts
#        Graspan's points-to facts of real C programs (httpd 2.2.18, Linux 4.4-rc5,
#        PostgreSQL 8.3.9), the same files the GPUlog paper used. Taken from harp-lab/gdlog-HIP
#        because in harp-lab/gdlog they are Git LFS pointers that cannot be downloaded.
#   graphs/<name>/edge.facts
#        the TC / SG benchmark graphs of the GPUlog paper (SuiteSparse / SNAP originals)
set -e
DATA=${1:-${STADATA:-$HOME/stadata}}
BASE=https://raw.githubusercontent.com/harp-lab/gdlog-HIP/main/data
mkdir -p "$DATA/cspa" "$DATA/graphs" "$DATA/downloads"

if [ ! -f "$DATA/cspa/httpd/assign.facts" ]; then
    curl -sSL -o "$DATA/downloads/cspa_datasets.zip" "$BASE/dataset/cspa_datasets.zip"
    unzip -q -o "$DATA/downloads/cspa_datasets.zip" -d "$DATA/cspa_tmp"
    # the zip may contain a top folder; find the program folders wherever they are
    for p in httpd linux postgresql; do
        d=$(find "$DATA/cspa_tmp" -type d -name "$p" | head -1)
        mkdir -p "$DATA/cspa/$p"
        cp "$d/assign.facts" "$d/dereference.facts" "$DATA/cspa/$p/"
    done
    rm -rf "$DATA/cspa_tmp"
fi

# name : file in gdlog-HIP/data
GRAPHS="fe_body:data_163734.txt SF.cedge:data_223001.txt p2p-Gnutella04:data_39994.txt
cal.cedge:data_21693.txt ego-Facebook:data_88234.txt CA-HepTh:data_51971.txt
loc-Brightkite:data_214078.txt fe_sphere:data_49152.txt"
for g in $GRAPHS; do
    name=${g%%:*}
    file=${g#*:}
    out="$DATA/graphs/$name/edge.facts"
    [ -f "$out" ] && continue
    mkdir -p "$DATA/graphs/$name"
    # keep only the first two columns, tab separated
    curl -sSL "$BASE/$file" | awk 'NF >= 2 {print $1 "\t" $2}' > "$out"
done

for f in "$DATA"/cspa/*/*.facts "$DATA"/graphs/*/edge.facts; do
    printf "%-60s %10d lines\n" "$f" "$(wc -l < "$f")"
done
