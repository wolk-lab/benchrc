mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    common::{histogram_uniform_10m, histogram_uniform_1m},
    histogram::{rayon as histogram_rayon, sequential as histogram_seq},
};
use std::hint::black_box;

fn histogram_benches(c: &mut criterion::Criterion) {
    let cases = [
        ("1M_uniform_256", histogram_uniform_1m().as_slice(), 256usize),
        ("10M_uniform_256", histogram_uniform_10m().as_slice(), 256usize),
    ];

    let mut group = c.benchmark_group("histogram");
    configure_group(&mut group);

    for (label, data, buckets) in &cases {
        set_element_throughput(&mut group, data.len());

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

criterion_group!(name = histogram; config = default_criterion(); targets = histogram_benches);
criterion_main!(histogram);
