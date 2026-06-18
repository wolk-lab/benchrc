import os
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "plots")
DATA_DIR = os.path.join(OUT_DIR, "raw")
THREADS = [1, 2, 4, 8, 16, 32]

MACHINE_INFO = {
    "Host": "amd147.utah.cloudlab.us",
    "CPU": "AMD EPYC 7302P 16-Core Processor",
    "Cores / Threads": "16 / 32",
    "RAM": "125 GiB",
    "OS": "Ubuntu 24.04",
    "Kernel": "6.8.0-117-generic",
    "GCC": "13.3.0",
    "Rust": "1.96.0",
}


def to_s(value: str, unit: str) -> float:
    scale = {
        "ns": 1e-9,
        "us": 1e-6,
        "µs": 1e-6,
        "μs": 1e-6,
        "ms": 1e-3,
        "s": 1.0,
    }[unit]
    return float(value) * scale


def parse_rust(path: str):
    """Return dict[name] = {mid_s, low_s, high_s}."""
    if not os.path.isfile(path):
        return {}

    with open(path) as f:
        text = f.read()

    pat = (
        r"(\S[\S/]+?)\s*\n\s+time:\s+\[([\d.]+)\s+(\S+)\s+([\d.]+)\s+\S+\s+([\d.]+)\s+\S+\s*\]"
        r"|(\S[\S/]+?)\s+time:\s+\[([\d.]+)\s+(\S+)\s+([\d.]+)\s+\S+\s+([\d.]+)\s+\S+\s*\]"
    )

    data = {}
    for match in re.finditer(pat, text):
        if match.group(1) is not None:
            name, low, unit, mid, high = (
                match.group(1),
                match.group(2),
                match.group(3),
                match.group(4),
                match.group(5),
            )
        else:
            name, low, unit, mid, high = (
                match.group(6),
                match.group(7),
                match.group(8),
                match.group(9),
                match.group(10),
            )

        data[name] = {
            "mid_s": to_s(mid, unit),
            "low_s": to_s(low, unit),
            "high_s": to_s(high, unit),
        }
    return data


def parse_cpp(path: str):
    """Return dict[(kind, variant, threads)] = {mean_s, stddev_s}."""
    if not os.path.isfile(path):
        return {}

    with open(path) as f:
        lines = f.readlines()

    data = {}

    threaded_mean = re.compile(r"^(\w+)/(\w+)/(\d+)/.*real_time_mean\s+([\d.eE+\-]+)\s+ns")
    threaded_std = re.compile(r"^(\w+)/(\w+)/(\d+)/.*real_time_stddev\s+([\d.eE+\-]+)\s+ns")
    seq_mean = re.compile(r"^(\w+)/(\w+)/.*real_time_mean\s+([\d.eE+\-]+)\s+ns")
    seq_std = re.compile(r"^(\w+)/(\w+)/.*real_time_stddev\s+([\d.eE+\-]+)\s+ns")

    for line in lines:
        m = threaded_mean.match(line)
        if m:
            key = (m.group(1), m.group(2), int(m.group(3)))
            data.setdefault(key, {})["mean_s"] = float(m.group(4)) / 1e9
            continue

        m = threaded_std.match(line)
        if m:
            key = (m.group(1), m.group(2), int(m.group(3)))
            data.setdefault(key, {})["stddev_s"] = float(m.group(4)) / 1e9
            continue

        m = seq_mean.match(line)
        if m and m.group(1).endswith("Seq"):
            key = (m.group(1), m.group(2), None)
            data.setdefault(key, {})["mean_s"] = float(m.group(3)) / 1e9
            continue

        m = seq_std.match(line)
        if m and m.group(1).endswith("Seq"):
            key = (m.group(1), m.group(2), None)
            data.setdefault(key, {})["stddev_s"] = float(m.group(3)) / 1e9

    return data


rust_hist = parse_rust(os.path.join(DATA_DIR, "rust_histogram.txt"))
rust_merge = parse_rust(os.path.join(DATA_DIR, "rust_mergesort.txt"))
rust_stencil = parse_rust(os.path.join(DATA_DIR, "rust_stencil.txt"))
rust_bfs = parse_rust(os.path.join(DATA_DIR, "rust_bfs.txt"))
rust_rayon = parse_rust(os.path.join(DATA_DIR, "rust_rayon_overhead.txt"))

cpp_hist = parse_cpp(os.path.join(DATA_DIR, "cpp_histogram.txt"))
cpp_merge = parse_cpp(os.path.join(DATA_DIR, "cpp_mergesort.txt"))
cpp_stencil = parse_cpp(os.path.join(DATA_DIR, "cpp_stencil.txt"))
cpp_bfs = parse_cpp(os.path.join(DATA_DIR, "cpp_bfs.txt"))

FAMILIES = [
    {
        "label": "Histogram 1M",
        "slug": "histogram_1m",
        "kernel": "histogram",
        "rust_dataset": "1M_uniform_256",
        "cpp_dataset": "1M",
        "cpp_prefix": "Histogram",
        "rust": rust_hist,
        "cpp": cpp_hist,
        "y_lim": None,
        "y_scale": "linear",
        "y_unit": "ms",
    },
    {
        "label": "Histogram 1B",
        "slug": "histogram_1b",
        "kernel": "histogram",
        "rust_dataset": "1B_uniform_256",
        "cpp_dataset": "1B",
        "cpp_prefix": "Histogram",
        "rust": rust_hist,
        "cpp": cpp_hist,
        "y_lim": None,
        "y_scale": "linear",
        "y_unit": "s",
    },
    {
        "label": "Mergesort 1B",
        "slug": "mergesort_1b",
        "kernel": "mergesort",
        "rust_dataset": "1B_u32_cutoff1024",
        "cpp_dataset": "1B",
        "cpp_prefix": "Mergesort",
        "rust": rust_merge,
        "cpp": cpp_merge,
        "y_lim": None,
        "y_scale": "linear",
        "y_unit": "s",
    },
    {
        "label": "Stencil 1B",
        "slug": "stencil_1b",
        "kernel": "stencil",
        "rust_dataset": "1B_k10_r1",
        "cpp_dataset": "1B",
        "cpp_prefix": "Stencil",
        "rust": rust_stencil,
        "cpp": cpp_stencil,
        "y_lim": None,
        "y_scale": "linear",
        "y_unit": "s",
    },
    {
        "label": "BFS 320M",
        "slug": "bfs_320m",
        "kernel": "bfs",
        "rust_dataset": "320M_tree",
        "cpp_dataset": "320M",
        "cpp_prefix": "Bfs",
        "rust": rust_bfs,
        "cpp": cpp_bfs,
        "y_lim": None,
        "y_scale": "linear",
        "y_unit": "s",
    },
]


def rust_seq_key(fam):
    return f"{fam['kernel']}/seq/{fam['rust_dataset']}"


def rust_rayon_key(fam, threads):
    return f"{fam['kernel']}/rayon_{threads}t/{fam['rust_dataset']}"


def cpp_seq_key(fam):
    return (f"{fam['cpp_prefix']}Seq", fam["cpp_dataset"], None)


def cpp_thread_key(fam, suffix, threads):
    return (f"{fam['cpp_prefix']}{suffix}", fam["cpp_dataset"], threads)


def plot_family(fam):
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    x = np.array(THREADS)

    scale = 1000.0 if fam.get("y_unit") == "ms" else 1.0
    y_label = f"Time ({fam.get('y_unit', 's')}) — lower is better"

    rust_seq = fam["rust"].get(rust_seq_key(fam))
    cpp_seq = fam["cpp"].get(cpp_seq_key(fam))

    if rust_seq:
        ax.axhline(rust_seq["mid_s"] * scale, color="#666666", linestyle="--", linewidth=1.2,
               label=f"Rust seq ({rust_seq['mid_s'] * scale:.2f} {fam.get('y_unit', 's')})")

    if cpp_seq and "mean_s" in cpp_seq:
        mean = cpp_seq["mean_s"] * scale
        std = (cpp_seq.get("stddev_s", 0.0) or 0.0) * scale
        ax.axhline(mean, color="#111111", linestyle=":", linewidth=1.2,
                   label=f"C++ seq ({mean:.2f} {fam.get('y_unit', 's')})")

    series = [
        ("Rust rayon", "#e24a33", "o-", [fam["rust"].get(rust_rayon_key(fam, t)) for t in THREADS], "rust"),
        ("C++ OpenMP", "#348abd", "s-", [fam["cpp"].get(cpp_thread_key(fam, 'OpenMP', t)) for t in THREADS], "cpp"),
        ("C++ Taskflow", "#988ed5", "^-", [fam["cpp"].get(cpp_thread_key(fam, 'Taskflow', t)) for t in THREADS], "cpp"),
    ]

    for label, color, fmt, points, kind in series:
        valid = [(t, p) for t, p in zip(THREADS, points) if p is not None]
        if not valid:
            continue
        xs = np.array([t for t, _ in valid])
        if kind == "rust":
            mids = np.array([p["mid_s"] * scale for _, p in valid])
            lows = np.array([(p["mid_s"] - p["low_s"]) * scale for _, p in valid])
            highs = np.array([(p["high_s"] - p["mid_s"]) * scale for _, p in valid])
        else:
            mids = np.array([p["mean_s"] * scale for _, p in valid])
            stds = np.array([(p.get("stddev_s", 0.0) or 0.0) * scale for _, p in valid])
            lows = stds
            highs = stds

        ax.errorbar(
            xs,
            mids,
            yerr=[lows, highs],
            fmt=fmt,
            color=color,
            label=label,
            capsize=4,
            capthick=1,
            linewidth=1.6,
            markersize=6,
            markeredgewidth=0.5,
            markeredgecolor="white",
        )

    ax.set_title(fam["label"])
    ax.set_xlabel("Threads")
    ax.set_ylabel(y_label)
    ax.set_xscale("log", base=2)
    ax.set_xticks(x)
    ax.set_xticklabels([str(t) for t in THREADS])
    ax.set_yscale(fam["y_scale"])
    if fam["y_lim"]:
        ax.set_ylim(*fam["y_lim"])
    ax.grid(axis="y", alpha=0.25)
    ax.legend(frameon=False, fontsize=8)
    plt.tight_layout()
    fig.savefig(os.path.join(OUT_DIR, f"{fam['slug']}.png"), dpi=160)
    plt.close(fig)


def fmt_rust_seq(fam):
    seq = fam["rust"].get(rust_seq_key(fam))
    if not seq:
        return "-", "-"
    return f"{seq['mid_s']:.3f}", f"[{seq['low_s']:.3f}, {seq['high_s']:.3f}]"


def fmt_cpp_seq(fam):
    seq = fam["cpp"].get(cpp_seq_key(fam))
    if not seq or "mean_s" not in seq:
        return "-", "-"
    std = seq.get("stddev_s", 0.0) or 0.0
    return f"{seq['mean_s']:.3f}", f"± {std:.3f}"


def family_rows(fam):
    rows = []
    for t in THREADS:
        rust = fam["rust"].get(rust_rayon_key(fam, t))
        omp = fam["cpp"].get(cpp_thread_key(fam, "OpenMP", t))
        tf = fam["cpp"].get(cpp_thread_key(fam, "Taskflow", t))
        rows.append(
            {
                "threads": t,
                "rust": rust,
                "omp": omp,
                "tf": tf,
            }
        )
    return rows


def write_report(families):
    report_path = os.path.join(OUT_DIR, "REPORT.md")
    with open(report_path, "w") as f:
        f.write("# benchrc Benchmark Report\n\n")
        f.write("## Machine\n\n")
        for key, value in MACHINE_INFO.items():
            f.write(f"- **{key}**: {value}\n")

        f.write("\n## Method\n\n")
        f.write("- Rust: Criterion, 5 samples, 10s warmup, 60s measurement.\n")
        f.write("- C++: Google Benchmark, Release build, 5 repetitions, 10s warmup, 60s min-time, real-time mode.\n")
        f.write("- Thread counts tested: " + ", ".join(str(t) for t in THREADS) + "\n")

        for fam in families:
            f.write(f"\n## {fam['label']}\n\n")
            rust_seq, rust_seq_ci = fmt_rust_seq(fam)
            cpp_seq, cpp_seq_std = fmt_cpp_seq(fam)
            f.write(f"- Rust seq: `{rust_seq} s`, CI `{rust_seq_ci}`\n")
            f.write(f"- C++ seq: `{cpp_seq} s`, stddev `{cpp_seq_std}`\n\n")
            f.write("| Threads | Rust rayon s | C++ OpenMP s | C++ Taskflow s |\n")
            f.write("|---|---:|---:|---:|\n")
            for row in family_rows(fam):
                rust = row["rust"]
                omp = row["omp"]
                tf = row["tf"]
                rust_cell = f"{rust['mid_s']:.3f}" if rust else "-"
                omp_cell = f"{omp['mean_s']:.3f}" if omp else "-"
                tf_cell = f"{tf['mean_s']:.3f}" if tf else "-"
                f.write(f"| {row['threads']} | {rust_cell} | {omp_cell} | {tf_cell} |\n")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for fam in FAMILIES:
        plot_family(fam)
        print(f"generated {fam['slug']}.png")
    write_report(FAMILIES)
    print(f"wrote {os.path.join(OUT_DIR, 'REPORT.md')}")


if __name__ == "__main__":
    main()
