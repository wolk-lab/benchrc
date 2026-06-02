#!/usr/bin/env python3
"""Pre-generate all datasets as raw binary files for deterministic benchmarking."""

import os
import struct
import random
import math
import sys

DATASETS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "datasets")

# Seeded RNG for reproducibility
random.seed(42)


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def write_u64(path, data):
    with open(path, "wb") as f:
        for v in data:
            f.write(struct.pack("<Q", v))


def write_u32(path, data):
    with open(path, "wb") as f:
        for v in data:
            f.write(struct.pack("<I", v))


def write_f64(path, data):
    with open(path, "wb") as f:
        for v in data:
            f.write(struct.pack("<d", v))


def generate_uniform(n, buckets):
    return [random.randrange(0, buckets) for _ in range(n)]


def generate_zipf(n, buckets, alpha=1.5):
    """Generate zipf-distributed samples using CDF + binary search."""
    cdf = [0.0] * buckets
    denom = sum(1.0 / (i + 1) ** alpha for i in range(buckets))
    cum = 0.0
    for i in range(buckets):
        cum += 1.0 / (i + 1) ** alpha / denom
        cdf[i] = cum

    data = []
    for _ in range(n):
        u = random.random()
        lo, hi = 0, buckets - 1
        while lo < hi:
            mid = lo + (hi - lo) // 2
            if cdf[mid] <= u:
                lo = mid + 1
            else:
                hi = mid
        data.append(lo)
    return data


def generate_u32(n):
    return [random.getrandbits(32) for _ in range(n)]


def generate_f64(n):
    return [random.random() for _ in range(n)]


def generate_strings(n):
    chars = "abcdefghijklmnopqrstuvwxyz0123456789"
    data = []
    for _ in range(n):
        length = random.randint(8, 64)
        s = "".join(random.choice(chars) for _ in range(length))
        data.append(s)
    return data


def write_strings(path, data):
    """Write strings as: N (u64) followed by [len (u64), bytes...] for each."""
    with open(path, "wb") as f:
        f.write(struct.pack("<Q", len(data)))
        for s in data:
            encoded = s.encode("utf-8")
            f.write(struct.pack("<Q", len(encoded)))
            f.write(encoded)


def main():
    print(f"Generating datasets in {DATASETS_DIR}...", flush=True)

    hist_sizes = [1_000_000, 10_000_000, 100_000_000]
    hist_buckets_list = [256, 65536]

    # Histogram datasets
    ensure_dir(os.path.join(DATASETS_DIR, "histogram"))
    for n in hist_sizes:
        for buckets in hist_buckets_list:
            for dist in ["uniform", "zipf"]:
                label = f"{n}_{dist}_{buckets}"
                path = os.path.join(DATASETS_DIR, "histogram", f"{label}.bin")
                if os.path.exists(path):
                    print(f"  [skip] histogram/{label}.bin", flush=True)
                    continue
                print(f"  generating histogram/{label}.bin  (n={n}, buckets={buckets})", flush=True)
                if dist == "uniform":
                    data = generate_uniform(n, buckets)
                else:
                    data = generate_zipf(n, buckets)
                write_u64(path, data)

    # Mergesort u32 datasets
    ensure_dir(os.path.join(DATASETS_DIR, "mergesort"))
    for n in hist_sizes:
        label = f"{n}_u32"
        path = os.path.join(DATASETS_DIR, "mergesort", f"{label}.bin")
        if not os.path.exists(path):
            print(f"  generating mergesort/{label}.bin  (n={n})", flush=True)
            data = generate_u32(n)
            write_u32(path, data)

    # Mergesort String datasets
    for n in [10_000_000]:
        label = f"{n}_String"
        path = os.path.join(DATASETS_DIR, "mergesort", f"{label}.bin")
        if not os.path.exists(path):
            print(f"  generating mergesort/{label}.bin  (n={n})", flush=True)
            data = generate_strings(n)
            write_strings(path, data)

    # Stencil datasets
    ensure_dir(os.path.join(DATASETS_DIR, "stencil"))
    for n in hist_sizes:
        label = f"{n}"
        path = os.path.join(DATASETS_DIR, "stencil", f"{label}.bin")
        if not os.path.exists(path):
            print(f"  generating stencil/{label}.bin  (n={n})", flush=True)
            data = generate_f64(n)
            write_f64(path, data)

    print("All datasets generated.", flush=True)


if __name__ == "__main__":
    main()
