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
HISTOGRAM_1B = 1_000_000_000
MERGESORT_1B = 1_000_000_000
STENCIL_1B = 1_000_000_000
RAYON_OVERHEAD_10K = 10_000
BFS_320M_NODES = 320_000_000
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
    """Generate a binary-tree graph: node i has children 2i+1 and 2i+2.
    Wide frontiers enable parallel BFS scaling."""
    offsets = array.array("Q", [0])
    edges = array.array("I")

    for node in range(num_nodes):
        left = 2 * node + 1
        right = 2 * node + 2
        if left < num_nodes:
            edges.append(left)
        if right < num_nodes:
            edges.append(right)
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

    # 1B / 6M scaled datasets
    _generate("histogram_uniform_1b.u64le.bin", output_dir, lambda path: _write_histogram_dataset(path, HISTOGRAM_1B))
    _generate("mergesort_u32_1b.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, MERGESORT_1B, SEED))
    _generate("stencil_f64_1b.f64le.bin", output_dir, lambda path: _write_f64_dataset(path, STENCIL_1B, SEED))
    _generate("rayon_overhead_u32_10k.u32le.bin", output_dir, lambda path: _write_u32_dataset(path, RAYON_OVERHEAD_10K, SEED))
    _generate("bfs_320m_tree.graph.bin", output_dir, lambda path: _write_bfs_graph(path, BFS_320M_NODES, BFS_FANOUT))


if __name__ == "__main__":
    main()
