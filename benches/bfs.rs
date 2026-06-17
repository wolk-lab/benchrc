mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    bfs::{rayon as bfs_rayon, sequential as bfs_seq},
    common::bfs_graph_6m,
};
use std::hint::black_box;

fn bench_bfs_group(group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>, label: &str, graph: &benchrc::benchmarks::common::Graph) {
    set_element_throughput(group, graph.num_nodes);

    group.bench_function(BenchmarkId::new("seq", label), |b| {
        b.iter(|| black_box(bfs_seq::bfs(black_box(graph), 0)));
    });

    for &threads in &RAYON_THREAD_COUNTS {
        let pool = rayon::ThreadPoolBuilder::new()
            .num_threads(threads)
            .build()
            .unwrap();
        group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), label), |b| {
            b.iter(|| {
                pool.install(|| black_box(bfs_rayon::bfs(black_box(graph), 0)));
            });
        });
    }
}

fn bfs_benches(c: &mut criterion::Criterion) {
    let mut group = c.benchmark_group("bfs");
    configure_group(&mut group);

    bench_bfs_group(&mut group, "6M_fanout4", bfs_graph_6m());

    group.finish();
}

criterion_group!(name = bfs; config = default_criterion(); targets = bfs_benches);
criterion_main!(bfs);
