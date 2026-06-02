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
- Nix development shell provisions the required toolchain

See [BENCHMARKS.md](./BENCHMARKS.md) for setup and run instructions.
