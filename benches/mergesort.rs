mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BatchSize, BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    common::mergesort_u32_1b,
    mergesort::{rayon as mergesort_rayon, sequential as mergesort_seq},
};
use std::hint::black_box;

fn bench_mergesort_group(group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>, label: &str, source: &[u32]) {
    set_element_throughput(group, source.len());

    group.bench_function(BenchmarkId::new("seq", label), |b| {
        b.iter_batched(
            || source.to_vec(),
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
        group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), label), |b| {
            b.iter_batched(
                || source.to_vec(),
                |mut values| {
                    pool.install(|| mergesort_rayon::mergesort_u32(black_box(&mut values), 1024));
                    black_box(values);
                },
                BatchSize::LargeInput,
            );
        });
    }
}

fn mergesort_benches(c: &mut criterion::Criterion) {
    let mut group = c.benchmark_group("mergesort");
    configure_group(&mut group);

    bench_mergesort_group(&mut group, "1B_u32_cutoff1024", mergesort_u32_1b());

    group.finish();
}

criterion_group!(name = mergesort; config = default_criterion(); targets = mergesort_benches);
criterion_main!(mergesort);
