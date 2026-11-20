# Running Person C's GPU version on Google Colab

Our laptops have no NVIDIA GPU. The Thrust code (`cuda/thrust_backend.h`) is already tested on
the CPU (`bin/thrust_omp`, `make test`); on Colab we compile the same file with nvcc.

1. On your machine (WSL), in the repo root: `git archive -o parallel_datalog.zip HEAD parallel_datalog`
2. New Colab notebook, **Runtime > Change runtime type > T4 GPU**, upload `parallel_datalog.zip`, then:

```bash
!nvidia-smi
!unzip -q -o parallel_datalog.zip && cd parallel_datalog && make bin/seq bin/omp bin/relhash gpu NVFLAGS=-arch=sm_75
```

```bash
# inputs: real-program CSPA facts + benchmark graphs (~2 min download)
!cd parallel_datalog && STADATA=/content/stadata bash tools/download_data.sh /content/stadata && \
  for p in httpd linux postgresql; do mkdir -p /content/stadata/inputs/cspa_$p && \
  ln -sf /content/stadata/cspa/$p/*.facts /content/stadata/inputs/cspa_$p/; done && \
  for g in SF.cedge fe_body; do mkdir -p /content/stadata/inputs/tc_$g && \
  ln -sf /content/stadata/graphs/$g/edge.facts /content/stadata/inputs/tc_$g/; done && \
  mkdir -p /content/stadata/inputs/sg_ego-Facebook && \
  ln -sf /content/stadata/graphs/ego-Facebook/edge.facts /content/stadata/inputs/sg_ego-Facebook/
```

```bash
# GPU vs our CPU engines on the same (2-core) Colab machine; every result is checked
!cd parallel_datalog && STADATA=/content/stadata python3 scripts/bench.py --engines seq,omp3,thrust_gpu \
   --threads 2 --inputs cspa_linux,cspa_httpd,tc_SF.cedge,tc_fe_body,sg_ego-Facebook --out results/bench/bench_gpu.csv
```

3. Download `parallel_datalog/results/bench/bench_gpu.csv`, put it in `results/bench/` here.

`solve_s` of `thrust_gpu` is the time on the device (all relations stay in GPU memory between
rounds); `comm_s` is the host-to-device copy of the input facts. Compare `solve_s` and `wall_s`
separately, as the project plan asks (kernel-only vs end-to-end).
