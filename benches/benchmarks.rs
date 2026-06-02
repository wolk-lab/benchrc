use criterion::{BenchmarkId, BatchSize, Criterion, Throughput, criterion_group, criterion_main};
use seminar::benchmarks::{
    bfs::{rayon as bfs_rayon, sequential as bfs_seq},
    common::{Graph, generate_f64, generate_u32, generate_uniform},
    histogram::{rayon as histogram_rayon, sequential as histogram_seq},
    mergesort::{rayon as mergesort_rayon, sequential as mergesort_seq},
    stencil::{rayon as stencil_rayon, sequential as stencil_seq},
};
use std::hint::black_box;
use std::time::Duration;

const RAYON_THREAD_COUNTS: [usize; 4] = [1, 2, 4, 8];

fn configure_group(group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>) {
    group.sample_size(30);
    group.warm_up_time(Duration::from_secs(3));
    group.measurement_time(Duration::from_secs(10));
}

fn benchmark_graph(num_nodes: usize, fanout: usize) -> Graph {
    let mut offsets = Vec::with_capacity(num_nodes + 1);
    let mut edges = Vec::new();
    offsets.push(0);

    for node in 0..num_nodes {
        for step in 1..=fanout {
            let neighbor = node + step;
            if neighbor < num_nodes {
                edges.push(neighbor as u32);
            }
        }
        offsets.push(edges.len());
    }

    Graph {
        offsets,
        edges,
        num_nodes,
    }
}

fn histogram_benches(c: &mut Criterion) {
    let cases = vec![
        ("1M_uniform_256", generate_uniform(1_000_000, 256, 42), 256usize),
        ("10M_uniform_256", generate_uniform(10_000_000, 256, 42), 256usize),
    ];

    let mut group = c.benchmark_group("histogram");
    configure_group(&mut group);

    for (label, data, buckets) in &cases {
        group.throughput(Throughput::Elements(data.len() as u64));
        group.bench_function(BenchmarkId::new("seq", label), |b| {
            b.iter(|| black_box(histogram_seq::histogram(black_box(data), *buckets)));
        });

        for &threads in &RAYON_THREAD_COUNTS {
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(threads)
                .build()
                .unwrap();
            group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), label), |b| {
                b.iter(|| {
                    pool.install(|| black_box(histogram_rayon::histogram(black_box(data), *buckets)));
                });
            });
        }
    }

    group.finish();
}

fn mergesort_benches(c: &mut Criterion) {
    let source = generate_u32(1_000_000, 42);
    let mut group = c.benchmark_group("mergesort");
    configure_group(&mut group);
    group.throughput(Throughput::Elements(source.len() as u64));

    group.bench_function(BenchmarkId::new("seq", "1M_u32_cutoff1024"), |b| {
        b.iter_batched(
            || source.clone(),
            |mut values| {
                mergesort_seq::mergesort_u32(black_box(&mut values), 1024);
                black_box(values);
            },
            BatchSize::LargeInput,
        );
    });

    for &threads in &RAYON_THREAD_COUNTS {
        let pool = rayon::ThreadPoolBuilder::new()
            .num_threads(threads)
            .build()
            .unwrap();
        group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), "1M_u32_cutoff1024"), |b| {
            b.iter_batched(
                || source.clone(),
                |mut values| {
                    pool.install(|| mergesort_rayon::mergesort_u32(black_box(&mut values), 1024));
                    black_box(values);
                },
                BatchSize::LargeInput,
            );
        });
    }

    group.finish();
}

fn stencil_benches(c: &mut Criterion) {
    let data = generate_f64(1_000_000, 42);
    let mut group = c.benchmark_group("stencil");
    configure_group(&mut group);
    group.throughput(Throughput::Elements(data.len() as u64));

    group.bench_function(BenchmarkId::new("seq", "1M_k10_r1"), |b| {
        b.iter(|| black_box(stencil_seq::stencil(black_box(&data), 10, 1)));
    });

    for &threads in &RAYON_THREAD_COUNTS {
        let pool = rayon::ThreadPoolBuilder::new()
            .num_threads(threads)
            .build()
            .unwrap();
        group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), "1M_k10_r1"), |b| {
            b.iter(|| {
                pool.install(|| black_box(stencil_rayon::stencil(black_box(&data), 10, 1)));
            });
        });
    }

    group.finish();
}

fn bfs_benches(c: &mut Criterion) {
    let graph = benchmark_graph(65_536, 4);
    let mut group = c.benchmark_group("bfs");
    configure_group(&mut group);
    group.throughput(Throughput::Elements(graph.num_nodes as u64));

    group.bench_function(BenchmarkId::new("seq", "64K_fanout4"), |b| {
        b.iter(|| black_box(bfs_seq::bfs(black_box(&graph), 0)));
    });

    for &threads in &RAYON_THREAD_COUNTS {
        let pool = rayon::ThreadPoolBuilder::new()
            .num_threads(threads)
            .build()
            .unwrap();
        group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), "64K_fanout4"), |b| {
            b.iter(|| {
                pool.install(|| black_box(bfs_rayon::bfs(black_box(&graph), 0)));
            });
        });
    }

    group.finish();
}

criterion_group!(
    name = full_benchmarks;
    config = Criterion::default();
    targets = histogram_benches, mergesort_benches, stencil_benches, bfs_benches
);
criterion_main!(full_benchmarks);
