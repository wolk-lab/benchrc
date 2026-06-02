#!/usr/bin/env python3
import argparse
import csv
import json
import os
import shlex
import shutil
import subprocess
import tempfile

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
RUST_BIN = os.path.join(PROJECT_ROOT, "target", "release", "seminar")
CPP_DIR = os.path.join(PROJECT_ROOT, "cpp")
DATASETS_DIR = os.path.join(PROJECT_ROOT, "datasets")

HISTOGRAM_INPUTS = [
    "1000000_uniform_256",
    "10000000_uniform_256",
    "100000000_uniform_256",
    "1000000_zipf_256",
    "10000000_zipf_256",
    "100000000_zipf_256",
    "10000000_uniform_65536",
    "10000000_zipf_65536",
]

MERGESORT_INPUTS = [
    "1000000_u32_cutoff256",
    "1000000_u32_cutoff1024",
    "1000000_u32_cutoff4096",
    "1000000_u32_cutoff16384",
    "10000000_u32_cutoff256",
    "10000000_u32_cutoff1024",
    "10000000_u32_cutoff4096",
    "10000000_u32_cutoff16384",
    "100000000_u32_cutoff256",
    "100000000_u32_cutoff1024",
    "100000000_u32_cutoff4096",
    "100000000_u32_cutoff16384",
    "10000000_String_cutoff256",
    "10000000_String_cutoff1024",
    "10000000_String_cutoff4096",
    "10000000_String_cutoff16384",
]

STENCIL_INPUTS = [
    "1000000_k10_r1",
    "10000000_k10_r1",
    "100000000_k10_r1",
    "10000000_k100_r1",
    "10000000_k500_r1",
    "10000000_k10_r3",
    "10000000_k100_r3",
    "10000000_k500_r3",
]

THREAD_COUNTS = [1, 2, 4, 8]
NUM_RUNS = 20
WARMUP = 3


def dataset_path(benchmark, input_spec):
    if benchmark == "histogram":
        n, dist, buckets = input_spec.split("_")
        filename = f"{n}_{dist}_{buckets}.bin"
    elif benchmark == "mergesort":
        n, dtype, _ = input_spec.split("_")
        filename = f"{n}_{dtype}.bin"
    elif benchmark == "stencil":
        filename = f"{input_spec.split('_')[0]}.bin"
    else:
        return None

    path = os.path.join(DATASETS_DIR, benchmark, filename)
    return path if os.path.exists(path) else None


def build_all():
    subprocess.run(["cargo", "build", "--release"], cwd=PROJECT_ROOT, check=True, capture_output=True)
    subprocess.run(["make", "-j4"], cwd=CPP_DIR, check=True, capture_output=True)


def ensure_hyperfine():
    if shutil.which("hyperfine") is None:
        raise SystemExit("hyperfine is required but was not found in PATH")


def parse_csv_line(line):
    parts = line.strip().split(",")
    if len(parts) < 7:
        return None
    return {
        "benchmark": parts[0],
        "input": parts[1],
        "implementation": parts[2],
        "threads": int(parts[3]),
        "run": int(parts[4]),
        "time_seconds": float(parts[5]),
        "checksum": parts[6],
    }


def build_env(benchmark, input_spec):
    env = os.environ.copy()
    env["DYLD_LIBRARY_PATH"] = "/opt/homebrew/opt/libomp/lib"
    data_file = dataset_path(benchmark, input_spec)
    if data_file:
        env["DATA_FILE"] = data_file
    return env


def build_rust_command(benchmark, input_spec, impl, threads, run_id):
    return [
        RUST_BIN,
        "-b",
        benchmark,
        "-i",
        input_spec,
        "-m",
        impl,
        "-t",
        str(threads),
        "-r",
        str(run_id),
    ]


def build_cpp_command(benchmark, input_spec, impl, threads, run_id):
    binary = os.path.join(CPP_DIR, benchmark, impl)
    if benchmark == "histogram":
        n, dist, buckets = input_spec.split("_")
        return [binary, n, buckets, dist, str(threads), str(run_id)]
    if benchmark == "mergesort":
        n, dtype, cutoff = input_spec.split("_")
        return [binary, n, dtype, cutoff.removeprefix("cutoff"), str(threads), str(run_id)]
    if benchmark == "stencil":
        n, iterations, radius = input_spec.split("_")
        return [binary, n, iterations.removeprefix("k"), radius.removeprefix("r"), str(threads), str(run_id)]
    if benchmark == "bfs":
        return [binary, input_spec, str(threads), str(run_id)]
    raise ValueError(f"unknown benchmark: {benchmark}")


def build_command(benchmark, input_spec, impl, threads, run_id):
    if impl.startswith("rust_"):
        return build_rust_command(benchmark, input_spec, impl, threads, run_id)
    return build_cpp_command(benchmark, input_spec, impl.split("_", 1)[1], threads, run_id)


def run_once(benchmark, input_spec, impl, threads, run_id):
    command = build_command(benchmark, input_spec, impl, threads, run_id)
    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        timeout=300,
        env=build_env(benchmark, input_spec),
    )
    if result.returncode != 0:
        print(f"  FAILED: {result.stderr.strip()}", flush=True)
        return None
    for line in result.stdout.strip().splitlines():
        parsed = parse_csv_line(line)
        if parsed:
            return parsed
    return None


def record_checksum(expected_checksums, result):
    key = (result["benchmark"], result["input"])
    checksum = result["checksum"]
    previous = expected_checksums.setdefault(key, checksum)
    if previous != checksum:
        print(
            f"  WARNING: checksum mismatch across implementations for {result['benchmark']} {result['input']}: "
            f"{previous} != {checksum}",
            flush=True,
        )


def hyperfine_benchmark(benchmark, input_spec, impl, threads, expected_checksums):
    validation = run_once(benchmark, input_spec, impl, threads, 1)
    if validation is None:
        return None
    record_checksum(expected_checksums, validation)

    command = build_command(benchmark, input_spec, impl, threads, 1)
    hyperfine_command = shlex.join(command)
    env = build_env(benchmark, input_spec)

    with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as temp_file:
        json_path = temp_file.name

    try:
        result = subprocess.run(
            [
                "hyperfine",
                "--shell=none",
                "--warmup",
                str(WARMUP),
                "--runs",
                str(NUM_RUNS),
                "--export-json",
                json_path,
                hyperfine_command,
            ],
            capture_output=True,
            text=True,
            timeout=600,
            env=env,
        )
        if result.returncode != 0:
            print(f"  FAILED: {result.stderr.strip()}", flush=True)
            return None

        with open(json_path) as file:
            payload = json.load(file)
        stats = payload["results"][0]
        return {
            "mean": stats["mean"],
            "stddev": stats["stddev"],
            "median": stats["median"],
            "min": stats["min"],
            "max": stats["max"],
            "checksum": validation["checksum"],
        }
    finally:
        os.unlink(json_path)


def thread_counts_for(impl, small):
    if impl == "rust_seq":
        return [1]
    return [1, 4] if small else THREAD_COUNTS


def inputs_for(benchmark, graphs_dir):
    if benchmark == "histogram":
        return HISTOGRAM_INPUTS
    if benchmark == "mergesort":
        return MERGESORT_INPUTS
    if benchmark == "stencil":
        return STENCIL_INPUTS
    if benchmark == "bfs":
        return sorted(
            os.path.join(graphs_dir, filename)
            for filename in os.listdir(graphs_dir)
            if filename.endswith(".graph")
        )
    raise ValueError(f"unknown benchmark: {benchmark}")


def main():
    parser = argparse.ArgumentParser(description="Seminar benchmark runner")
    parser.add_argument("--small", action="store_true", help="Run a quick subset")
    parser.add_argument("--output", default="results.csv", help="Output CSV path")
    parser.add_argument("--graphs", default=None, help="Directory with graph files for BFS")
    args = parser.parse_args()

    ensure_hyperfine()
    build_all()

    all_results = []
    expected_checksums = {}
    benchmarks = ["histogram", "mergesort", "stencil"]
    if args.graphs:
        benchmarks.append("bfs")
    implementations = ["rust_seq", "rust_rayon", "cpp_openmp", "cpp_taskflow"]

    for benchmark in benchmarks:
        print(f"\n{'=' * 60}\nBenchmark: {benchmark}\n{'=' * 60}", flush=True)
        inputs = inputs_for(benchmark, args.graphs)
        if args.small:
            inputs = inputs[:2]

        for input_spec in inputs:
            for impl in implementations:
                for threads in thread_counts_for(impl, args.small):
                    stats = hyperfine_benchmark(benchmark, input_spec, impl, threads, expected_checksums)
                    if stats is None:
                        all_results.append(
                            {
                                "benchmark": benchmark,
                                "input": input_spec,
                                "implementation": impl,
                                "threads": threads,
                                "run": 0,
                                "time_seconds": -1.0,
                                "checksum": "FAILED",
                            }
                        )
                        continue

                    all_results.append(
                        {
                            "benchmark": benchmark,
                            "input": input_spec,
                            "implementation": impl,
                            "threads": threads,
                            "run": 0,
                            "time_seconds": stats["median"],
                            "checksum": stats["checksum"],
                        }
                    )
                    print(
                        f"  {benchmark:12s} {input_spec:35s} {impl:15s} t={threads:2d}  "
                        f"{stats['median'] * 1000:.3f}ms ± {stats['stddev'] * 1000:.3f}ms  (n={NUM_RUNS})",
                        flush=True,
                    )

    fieldnames = ["benchmark", "input", "implementation", "threads", "run", "time_seconds", "checksum"]
    with open(args.output, "w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(all_results)

    print(f"\nResults written to {args.output}")
    print(f"Total data points: {len(all_results)}")


if __name__ == "__main__":
    main()
