### Soufflé profile: where the time goes

| input | threads | recursive rules (join + dedup check + insert into new) | merge new → full (serial copy) | other (non-recursive rules, loading) |
|---|---|---|---|---|
| cspa_httpd | 1 | 79.1% | 20.5% | 0.3% |
| cspa_httpd | 16 | 35.7% | 63.5% | 0.8% |
| cspa_linux | 1 | 86.0% | 8.6% | 5.4% |
| cspa_linux | 16 | 44.7% | 35.4% | 19.9% |
| taint_p2p-Gnutella04 | 1 | 85.0% | 13.1% | 1.9% |
| taint_p2p-Gnutella04 | 16 | 41.7% | 56.6% | 1.7% |
| tc_SF.cedge | 1 | 64.1% | 35.7% | 0.1% |
| tc_SF.cedge | 16 | 21.1% | 78.6% | 0.4% |

### Soufflé thread scaling (wall s / peak MB)

| input | serial code | par j1 | par j2 | par j4 | par j8 | par j16 |
|---|---|---|---|---|---|---|
| cspa_httpd | 116.3 / 8196 | 114.4 / 8196 | - | - | - | 41.9 / 9833 |
| cspa_linux | 43.5 / 1725 | 39.8 / 1725 | 27.1 / 1734 | 15.5 / 1829 | 13.8 / 1847 | 9.2 / 1840 |
| taint_p2p-Gnutella04 | 32.5 / 1440 | - | - | - | - | 8.8 / 1600 |
| tc_SF.cedge | 37.7 / 1995 | 37.2 / 1995 | - | - | - | 16.6 / 2008 |

### 1 thread: wall time in s (speedup of ours vs Soufflé)

| input | Soufflé serial | ours seq | speedup |
|---|---|---|---|
| cspa_bzip2 | 26.4 | 5.56 | 4.75x |
| cspa_httpd | 122.2 | 143.6 | 0.85x |
| cspa_linux | 44.1 | 22.1 | 2.00x |
| cspa_lua | 21.1 | 7.18 | 2.93x |
| tc_SF.cedge | 38.0 | 77.6 | 0.49x |
| sg_ego-Facebook | 318.7 | 35.0 | 9.12x |
| taint_httpd | 0.15 | 0.03 | 5.00x |
| taint_p2p-Gnutella04 | 32.6 | 13.3 | 2.46x |

### 16 threads / 16 MPI ranks: wall time in s

| input | Soufflé (parallel code) | omp v1 | omp v2 | omp v3 | mpi v1 | best ours vs Soufflé j16 | vs Soufflé serial |
|---|---|---|---|---|---|---|---|
| cspa_bzip2 | 3.79 | 2.33 | 2.51 | 2.57 | 3.20 | 1.63x (omp v1) | 11.3x |
| cspa_httpd | 50.9 | 38.0 | 47.1 | 43.0 | 25.0 | 2.04x (mpi v1) | 4.9x |
| cspa_linux | 11.8 | 8.08 | 8.00 | 7.81 | 5.69 | 2.08x (mpi v1) | 7.7x |
| cspa_lua | 3.05 | 3.01 | 2.93 | 2.97 | 3.59 | 1.04x (omp v2) | 7.2x |
| tc_SF.cedge | 17.2 | 28.7 | 22.0 | 6.30 | 20.6 | 2.73x (omp v3) | 6.0x |
| sg_ego-Facebook | 29.1 | 6.38 | 6.47 | 6.79 | 8.49 | 4.56x (omp v1) | 49.9x |
| taint_httpd | 0.12 | 0.04 | 0.04 | 0.04 | 1.69 | 3.00x (omp v1) | 3.8x |
| taint_p2p-Gnutella04 | 8.30 | 3.46 | 4.41 | 3.38 | 3.85 | 2.46x (omp v3) | 9.6x |

### Where our engines spend the solve time at 16 threads (s: join / sort+dedup / minus+merge / communication)

| input | omp v1 | omp v2 | omp v3 | mpi v1 |
|---|---|---|---|---|
| cspa_bzip2 | 0.3 / 0.9 / 0.1 / 0.0 | 0.4 / 0.3 / 1.1 / 0.0 | 0.3 / 0.4 / 0.9 / 0.0 | 0.1 / 0.3 / 0.0 / 1.3 |
| cspa_httpd | 2.6 / 6.0 / 28.5 / 0.0 | 2.6 / 4.9 / 39.0 / 0.0 | 2.5 / 8.1 / 30.5 / 0.0 | 1.4 / 3.2 / 10.7 / 7.4 |
| cspa_linux | 1.2 / 1.7 / 4.1 / 0.0 | 1.1 / 1.5 / 4.2 / 0.0 | 1.2 / 2.0 / 3.2 / 0.0 | 0.7 / 0.5 / 1.3 / 1.2 |
| cspa_lua | 0.3 / 1.1 / 0.2 / 0.0 | 0.3 / 0.3 / 1.0 / 0.0 | 0.3 / 0.4 / 0.9 / 0.0 | 0.1 / 0.4 / 0.0 / 1.2 |
| tc_SF.cedge | 0.4 / 1.1 / 29.5 / 0.0 | 0.4 / 1.0 / 22.6 / 0.0 | 0.4 / 1.5 / 4.1 / 0.0 | 0.1 / 0.2 / 12.2 / 6.5 |
| sg_ego-Facebook | 1.3 / 4.7 / 0.3 / 0.0 | 1.4 / 2.6 / 2.4 / 0.0 | 1.4 / 2.8 / 2.4 / 0.0 | 1.0 / 2.2 / 0.1 / 3.2 |
| taint_httpd | 0.0 / 0.0 / 0.0 / 0.0 | 0.0 / 0.0 / 0.0 / 0.0 | 0.0 / 0.0 / 0.0 / 0.0 | 0.0 / 0.0 / 0.0 / 0.0 |
| taint_p2p-Gnutella04 | 0.2 / 1.3 / 1.6 / 0.0 | 0.2 / 0.5 / 3.4 / 0.0 | 0.2 / 0.8 / 2.2 / 0.0 | 0.2 / 0.5 / 0.5 / 1.0 |

### Peak memory (MB) at 16 threads

| input | Soufflé (parallel code) | omp v1 | omp v2 | omp v3 | mpi v1 |
|---|---|---|---|---|---|
| cspa_bzip2 | 279 | 999 | 1062 | 993 | 166 |
| cspa_httpd | 9706 | 5955 | 14131 | 14248 | 495 |
| cspa_linux | 1845 | 1343 | 2756 | 2720 | 269 |
| cspa_lua | 262 | 1351 | 1392 | 1384 | 158 |
| tc_SF.cedge | 2010 | 1276 | 3629 | 3913 | 110 |
| sg_ego-Facebook | 553 | 4700 | 4872 | 4813 | 521 |
| taint_httpd | 21 | 24 | 24 | 24 | 43 |
| taint_p2p-Gnutella04 | 1600 | 1336 | 2415 | 2408 | 126 |
