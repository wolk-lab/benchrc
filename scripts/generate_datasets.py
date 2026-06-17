#!/usr/bin/env python3

import argparse
import array
import pathlib
import random
import struct
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = ROOT / "datasets"

HISTOGRAM_BUCKETS = 256
HISTOGRAM_1M = 1_000_000
HISTOGRAM_10M = 10_000_000
HISTOGRAM_1B = 1_000_000_000
MERGESORT_1M = 1_000_000
MERGESORT_1B = 1_000_000_000
STENCIL_1M = 1_000_000
STENCIL_1B = 1_000_000_000
RAYON_OVERHEAD_10K = 10_000
RAYON_OVERHEAD_1M = 1_000_000
BFS_NUM_NODES = 65_536
BFS_6M_NODES = 6_553_600
BFS_FANOUT = 4
SEED = 42


def _write_array(path: pathlib.Path, typecode: str, values) -> None:
    data = array.array(typecode, values)
    if sys.byteorder != "little":
        data.byteswap()
    with path.open("wb") as fh:
        data.tofile(fh)


def _write_histogram_dataset(path: pathlib.Path, count: int) -> None:
    rng = random.Random(SEED)
    _write_array(path, "Q", (rng.getrandbits(8) for _ in range(count)))


def _write_u32_dataset(path: pathlib.Path, count: int, seed: int) -> None:
    rng = random.Random(seed)
    _write_array(path, "I", (rng.getrandbits(32) for _ in range(count)))


def _write_f64_dataset(path: pathlib.Path, count: int, seed: int) -> None:
    rng = random.Random(seed)
    _write_array(path, "d", (rng.random() for _ in range(count)))


def _write_bfs_graph(path: pathlib.Path, num_nodes: int, fanout: int) -> None:
    offsets = array.array("Q", [0])
    edges = array.array("I")

    for node in range(num_nodes):
        for step in range(1, fanout + 1):
            neighbor = node + step
            if neighbor < num_nodes:
                edges.append(neighbor)
        offsets.append(len(edges))

    if sys.byteorder != "little":
        offsets.byteswap()
        edges.byteswap()

    with path.open("wb") as fh:
        fh.write(struct.pack("<QQQ", num_nodes, len(offsets), len(edges)))
        offsets.tofile(fh)
        edges.tofile(fh)


def _generate(name: str, output_dir: pathlib.Path, writer) -> None:
    path = output_dir / name
    started = time.time()
    writer(path)
    elapsed = time.time() - started
    size_mb = path.stat().st_size / (1024 * 1024)
    print(f"wrote {path} ({size_mb:.1f} MiB) in {elapsed:.2f}s")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate shared datasets for Rust and C++ benchmarks.")
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"directory to write datasets into (default: {DEFAULT_OUTPUT_DIR})",
    )
    args = parser.parse_args()

    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    _generate("histogram_uniform_1m.u64le.bin", output_dir, lambda path: _write_histogram_dataset(path, HISTOGRAM_1M))
    _generate("histogram_uniform_10m.u64le.bin", output_dir, lambda path: _write_histogram_dataset(path, HISTOGRAM_10M))
    _generate("mergesort_u32_1m.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, MERGESORT_1M, SEED))
    _generate("stencil_f64_1m.f64le.bin", output_dir, lambda path: _write_f64_dataset(path, STENCIL_1M, SEED))
    _generate("rayon_overhead_u32_10k.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, RAYON_OVERHEAD_10K, SEED))
    _generate("bfs_64k_fanout4.graph.bin", output_dir, lambda path: _write_bfs_graph(path, BFS_NUM_NODES, BFS_FANOUT))

    # 100x scaled datasets
    _generate("histogram_uniform_1b.u64le.bin", output_dir, lambda path: _write_histogram_dataset(path, HISTOGRAM_1B))
    _generate("mergesort_u32_1b.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, MERGESORT_1B, SEED))
    _generate("stencil_f64_1b.f64le.bin", output_dir, lambda path: _write_f64_dataset(path, STENCIL_1B, SEED))
    _generate("rayon_overhead_u32_1m.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, RAYON_OVERHEAD_1M, SEED))
    _generate("bfs_6m_fanout4.graph.bin", output_dir, lambda path: _write_bfs_graph(path, BFS_6M_NODES, BFS_FANOUT))


if __name__ == "__main__":
    main()
