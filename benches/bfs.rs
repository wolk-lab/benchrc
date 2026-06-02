mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    bfs::{rayon as bfs_rayon, sequential as bfs_seq},
    common::bfs_graph_64k,
};
use std::hint::black_box;

fn bfs_benches(c: &mut criterion::Criterion) {
    let graph = bfs_graph_64k();
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
