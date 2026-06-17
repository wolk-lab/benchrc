#!/usr/bin/env bash
#
# run_benchmarks.sh — Run the full benchrc suite with thread counts up to 32.
#
# Usage:
#   ./scripts/run_benchmarks.sh              # datasets + Rust + C++
#   ./scripts/run_benchmarks.sh --rust-only   # datasets + Rust only
#   ./scripts/run_benchmarks.sh --cpp-only    # datasets + C++ only
#   ./scripts/run_benchmarks.sh --data-only   # datasets only
#
# The script generates all datasets first, then runs the requested benchmarks.
# Each benchmark family writes its output to stdout and a tee'd log under plots/raw/.
#
# Environment variables:
#   BENCHRC_DATASETS_DIR   — override dataset directory (default: ./datasets)
#   SKIP_DATASETS          — if non-empty, skip dataset generation
#

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLOTS_RAW="${ROOT}/plots/raw"
DATASETS_DIR="${BENCHRC_DATASETS_DIR:-${ROOT}/datasets}"

mkdir -p "${PLOTS_RAW}"

# ── Dataset generation ──────────────────────────────────────────────────────
if [[ -z "${SKIP_DATASETS:-}" ]]; then
    echo "=== Generating datasets (100M variants included) ==="
    python3 "${ROOT}/scripts/generate_datasets.py" --output-dir "${DATASETS_DIR}"
    echo ""
else
    echo "=== Skipping dataset generation (SKIP_DATASETS is set) ==="
fi

# ── Helper: run a Rust benchmark and tee its output ─────────────────────────
run_rust_bench() {
    local bench_name="$1"
    local log_file="${PLOTS_RAW}/rust_${bench_name}.txt"
    echo ">>> Rust: cargo bench --bench ${bench_name}"
    cargo bench --bench "${bench_name}" 2>&1 | tee "${log_file}"
    local rc=$?
    echo ""
    return ${rc}
}

# ── Helper: configure, build, and run a C++ benchmark ─────────────────────
run_cpp_bench() {
    local target="$1"
    local log_file="${PLOTS_RAW}/cpp_${target%%_benchmarks}.txt"
    echo ">>> C++: building ${target}"
    cmake --build "${ROOT}/cpp/build" --target "${target}" 2>&1
    echo ">>> C++: running ./cpp/build/${target}"
    "${ROOT}/cpp/build/${target}" 2>&1 | tee "${log_file}"
    local rc=$?
    echo ""
    return ${rc}
}

# ── Rust benchmarks ────────────────────────────────────────────────────────
if [[ "${1:-}" != "--cpp-only" ]]; then
    echo "=============================================="
    echo "  Rust benchmarks (Criterion)"
    echo "=============================================="
    cd "${ROOT}"

    run_rust_bench "histogram"
    run_rust_bench "mergesort"
    run_rust_bench "stencil"
    run_rust_bench "bfs"
    run_rust_bench "rayon_overhead"
fi

# ── C++ benchmarks ──────────────────────────────────────────────────────────
if [[ "${1:-}" != "--rust-only" ]]; then
    echo "=============================================="
    echo "  C++ benchmarks (Google Benchmark)"
    echo "=============================================="
    cd "${ROOT}"

    # Configure CMake once if needed
    if [[ ! -d "${ROOT}/cpp/build" ]]; then
        cmake -S "${ROOT}/cpp" -B "${ROOT}/cpp/build"
    fi

    run_cpp_bench "histogram_benchmarks"
    run_cpp_bench "mergesort_benchmarks"
    run_cpp_bench "stencil_benchmarks"
    run_cpp_bench "bfs_benchmarks"
fi

echo "=== All benchmarks completed ==="
echo "Raw logs saved to ${PLOTS_RAW}/"
echo "Generate plots with: python3 ${ROOT}/scripts/plot_benchmarks.py"
