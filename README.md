# benchrc

`benchrc` is a benchmark suite for comparing Rust and C++ implementations of several parallel kernels.

Included benchmark families:

- histogram
- mergesort
- stencil
- bfs
- rayon overhead

Tooling:

- Rust benchmarks use Criterion
- C++ benchmarks use Google Benchmark

## Rust: Criterion

Run the Rust benchmark suites with Criterion:

```bash
cargo bench --bench histogram
cargo bench --bench mergesort
cargo bench --bench stencil
cargo bench --bench bfs
cargo bench --bench rayon_overhead
```

The Rust benchmarks are split by kernel family:

- `benches/histogram.rs`
- `benches/mergesort.rs`
- `benches/stencil.rs`
- `benches/bfs.rs`

Criterion handles warmup, sampling, and statistics in-process, so CLI parsing and process startup are excluded.

## C++: Google Benchmark

Configure and build the C++ suites:

```bash
cmake -S cpp -B cpp/build
cmake --build cpp/build --target histogram_benchmarks mergesort_benchmarks stencil_benchmarks bfs_benchmarks
```

Run them with:

```bash
./cpp/build/histogram_benchmarks
./cpp/build/mergesort_benchmarks
./cpp/build/stencil_benchmarks
./cpp/build/bfs_benchmarks
```

The C++ benchmarks are split by kernel family:

- `cpp/benchmarks/histogram.cpp`
- `cpp/benchmarks/mergesort.cpp`
- `cpp/benchmarks/stencil.cpp`
- `cpp/benchmarks/bfs.cpp`

Google Benchmark is configured to use real time and warmup so threaded C++ runs measure kernel wall-clock time.
