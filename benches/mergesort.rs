mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BatchSize, BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    common::{checksum_sum_u32, is_sorted_u32, mergesort_u32_1b},
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
    // Verify correctness before benchmarking
    let source = mergesort_u32_1b();
    let expected = checksum_sum_u32(source);
    let mut verify = source.to_vec();
    mergesort_seq::mergesort_u32(&mut verify, 1024);
    assert!(is_sorted_u32(&verify) && checksum_sum_u32(&verify) == expected, "sequential mergesort verification failed");
    let mut verify2 = source.to_vec();
    let pool = rayon::ThreadPoolBuilder::new().num_threads(4).build().unwrap();
    pool.install(|| mergesort_rayon::mergesort_u32(&mut verify2, 1024));
    assert!(is_sorted_u32(&verify2) && checksum_sum_u32(&verify2) == expected, "rayon mergesort verification failed");

    let mut group = c.benchmark_group("mergesort");
    configure_group(&mut group);

    bench_mergesort_group(&mut group, "1B_u32_cutoff1024", mergesort_u32_1b());

    group.finish();
}

criterion_group!(name = mergesort; config = default_criterion(); targets = mergesort_benches);
criterion_main!(mergesort);
