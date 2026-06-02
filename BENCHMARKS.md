# Benchmarks

## Development shell

Enter the benchmark environment with:

```bash
nix develop --no-pure-eval
```

That shell provides Rust, CMake, Google Benchmark, OpenMP, and Taskflow headers.

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

Configure and build the C++ suite:

```bash
cmake -S cpp -B cpp/build
cmake --build cpp/build --target seminar_google_benchmarks
```

Run it with:

```bash
./cpp/build/seminar_google_benchmarks
```

Useful filters:

```bash
./cpp/build/seminar_google_benchmarks --benchmark_filter=Histogram
./cpp/build/seminar_google_benchmarks --benchmark_filter=Mergesort
```

Google Benchmark is configured to use real time and warmup so threaded C++ runs measure kernel wall-clock time.
