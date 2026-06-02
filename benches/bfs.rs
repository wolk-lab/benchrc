mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use seminar::benchmarks::{
    bfs::{rayon as bfs_rayon, sequential as bfs_seq},
    common::Graph,
};
use std::hint::black_box;

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

fn bfs_benches(c: &mut criterion::Criterion) {
    let graph = benchmark_graph(65_536, 4);
    let mut group = c.benchmark_group("bfs");
    configure_group(&mut group);
    set_element_throughput(&mut group, graph.num_nodes);

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

criterion_group!(name = bfs; config = default_criterion(); targets = bfs_benches);
criterion_main!(bfs);
