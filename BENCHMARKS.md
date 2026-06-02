# Benchmarks

## Rust: Criterion

Run the Rust benchmark suite with Criterion:

```bash
cargo bench --bench benchmarks
cargo bench --bench rayon_overhead
```

The main Rust suite benchmarks the library kernels directly:

- histogram: sequential vs Rayon
- mergesort: sequential vs Rayon
- stencil: sequential vs Rayon
- bfs: sequential vs Rayon

Criterion handles warmup, sampling, and statistics in-process, so CLI parsing and process startup are excluded.

## C++: Google Benchmark

The C++ Google Benchmark target lives under `cpp/` and is built with CMake.

### Requirements

- CMake
- Google Benchmark with CMake package config
- OpenMP for OpenMP benchmarks
- Taskflow headers for Taskflow benchmarks

### Configure

```bash
cmake -S cpp -B cpp/build
```

### Build

```bash
cmake --build cpp/build --target seminar_google_benchmarks
```

### Run

```bash
./cpp/build/seminar_google_benchmarks
```

Useful filters:

```bash
./cpp/build/seminar_google_benchmarks --benchmark_filter=Histogram
./cpp/build/seminar_google_benchmarks --benchmark_filter=Mergesort
```

Google Benchmark is configured to use real time and warmup so threaded C++ runs measure kernel wall-clock time instead of process startup.

## Legacy CLI runners

The existing standalone Rust/C++ command-line runners still exist for end-to-end experiments, but the preferred kernel benchmark tools are:

- Rust: Criterion
- C++: Google Benchmark
