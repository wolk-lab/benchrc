mod common;

use benchrc::benchmarks::{
    common::stencil_f64_1b,
    stencil::{rayon as stencil_rayon, sequential as stencil_seq},
};
use common::{configure_group, default_criterion, set_element_throughput, RAYON_THREAD_COUNTS};
use criterion::{criterion_group, criterion_main, BenchmarkId};
use std::hint::black_box;

fn bench_stencil_group(group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>, label: &str, data: &[f64]) {
    set_element_throughput(group, data.len());

    group.bench_function(BenchmarkId::new("seq", label), |b| {
        b.iter(|| black_box(stencil_seq::stencil(black_box(data), 10, 1)));
    });

    for &threads in &RAYON_THREAD_COUNTS {
        let pool = rayon::ThreadPoolBuilder::new()
            .num_threads(threads)
            .build()
            .unwrap();
        group.bench_function(
            BenchmarkId::new(format!("rayon_{threads}t"), label),
            |b| {
                b.iter(|| {
                    pool.install(|| black_box(stencil_rayon::stencil(black_box(data), 10, 1)));
                });
            },
        );
    }
}

fn stencil_benches(c: &mut criterion::Criterion) {
    let mut group = c.benchmark_group("stencil");
    configure_group(&mut group);

    bench_stencil_group(&mut group, "1B_k10_r1", stencil_f64_1b());

    group.finish();
}

criterion_group!(name = stencil; config = default_criterion(); targets = stencil_benches);
criterion_main!(stencil);
