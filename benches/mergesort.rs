mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BatchSize, BenchmarkId, criterion_group, criterion_main};
use seminar::benchmarks::{
    common::generate_u32,
    mergesort::{rayon as mergesort_rayon, sequential as mergesort_seq},
};
use std::hint::black_box;

fn mergesort_benches(c: &mut criterion::Criterion) {
    let source = generate_u32(1_000_000, 42);
    let mut group = c.benchmark_group("mergesort");
    configure_group(&mut group);
    set_element_throughput(&mut group, source.len());

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

criterion_group!(name = mergesort; config = default_criterion(); targets = mergesort_benches);
criterion_main!(mergesort);
