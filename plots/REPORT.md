# benchrc Benchmark Report

## Machine

- **CPU**: AMD EPYC 7302P 16-Core Processor
- **Cores / Threads**: 16 / 32
- **RAM**: 125 GiB
- **OS**: Ubuntu 24.04
- **Kernel**: 6.8.0-117-generic
- **GCC**: 13.3.0
- **Rust**: 1.96.0

## Method

- Rust: Criterion, 30 samples, 3s warmup, 10s measurement.
- C++: Google Benchmark, Release build, 30 repetitions, 3s warmup, real-time mode.

## Histogram 1M

- Rust seq: `0.442 ms`, CI `[0.442, 0.442]`
- C++ seq: `0.498 ms`, stddev `± 0.051`

| Threads | Rust rayon ms | C++ OpenMP ms | C++ Taskflow ms |
|---|---:|---:|---:|
| 1 | 4.546 | 4.478 | 4.764 |
| 2 | 2.343 | 2.355 | 2.548 |
| 4 | 1.181 | 1.231 | 1.358 |
| 8 | 0.633 | 0.638 | 0.734 |

## Histogram 10M

- Rust seq: `7.637 ms`, CI `[7.632, 7.642]`
- C++ seq: `9.987 ms`, stddev `+- 0.216`

| Threads | Rust rayon ms | C++ OpenMP ms | C++ Taskflow ms |
|---|---:|---:|---:|
| 1 | 45.444 | 46.191 | 45.903 |
| 2 | 22.808 | 23.882 | 23.360 |
| 4 | 11.445 | 11.938 | 13.070 |
| 8 | 5.781 | 6.067 | 7.209 |

## Mergesort 1M

- Rust seq: `61.934 ms`, CI `[61.921, 61.947]`
- C++ seq: `83.183 ms`, stddev `+- 0.067`

| Threads | Rust rayon ms | C++ OpenMP ms | C++ Taskflow ms |
|---|---:|---:|---:|
| 1 | 60.064 | 82.650 | 83.757 |
| 2 | 33.716 | 44.863 | 47.819 |
| 4 | 21.194 | 35.093 | 29.804 |
| 8 | 16.501 | 23.014 | 20.476 |

## Stencil 1M

- Rust seq: `53.521 ms`, CI `[53.490, 53.555]`
- C++ seq: `22.904 ms`, stddev `+- 0.016`

| Threads | Rust rayon ms | C++ OpenMP ms | C++ Taskflow ms |
|---|---:|---:|---:|
| 1 | 58.830 | 24.052 | 29.280 |
| 2 | 30.521 | 12.975 | 16.408 |
| 4 | 17.916 | 7.723 | 10.236 |
| 8 | 10.636 | 5.337 | 6.838 |

## BFS 64K

- Rust seq: `0.583 ms`, CI `[0.583, 0.583]`
- C++ seq: `1.210 ms`, stddev `+- 0.044`

| Threads | Rust rayon ms | C++ OpenMP ms | C++ Taskflow ms |
|---|---:|---:|---:|
| 1 | 2.916 | 8.420 | 169.433 |
| 2 | 17.612 | 37.662 | 196.792 |
| 4 | 60.789 | 61.694 | 255.417 |
| 8 | 72.299 | 139.705 | 261.370 |
