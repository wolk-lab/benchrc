mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use seminar::benchmarks::{
    common::generate_f64,
    stencil::{rayon as stencil_rayon, sequential as stencil_seq},
};
use std::hint::black_box;

fn stencil_benches(c: &mut criterion::Criterion) {
    let data = generate_f64(1_000_000, 42);
    let mut group = c.benchmark_group("stencil");
    configure_group(&mut group);
    set_element_throughput(&mut group, data.len());

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

criterion_group!(name = stencil; config = default_criterion(); targets = stencil_benches);
criterion_main!(stencil);
